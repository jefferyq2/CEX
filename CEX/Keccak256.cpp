#include "Keccak256.h"
#include "Keccak.h"
#include "IntegerTools.h"
#include "MemoryTools.h"
#include "ParallelTools.h"

NAMESPACE_DIGEST

using Enumeration::DigestConvert;
using Utility::IntegerTools;
using Utility::MemoryTools;
using Utility::ParallelTools;

class Keccak256::Keccak256State
{
public:

	std::array<ulong, Keccak::KECCAK_STATE_SIZE> H = { 0 };

	Keccak256State()
	{
	}

	~Keccak256State()
	{
		Reset();
	}

	void Reset()
	{
		MemoryTools::Clear(H, 0, H.size() * sizeof(ulong));
	}
};

//~~~Constructor~~~//

Keccak256::Keccak256(bool Parallel)
	:
	m_dgtState(Parallel ? DEF_PRLDEGREE : 1),
	m_msgBuffer(Parallel ? DEF_PRLDEGREE * Keccak::KECCAK256_RATE_SIZE :
		Keccak::KECCAK256_RATE_SIZE),
	m_msgLength(0),
	m_parallelProfile(Keccak::KECCAK256_RATE_SIZE, Parallel, false, STATE_PRECACHED, false, DEF_PRLDEGREE),
	m_treeParams(Parallel ? KeccakParams(Keccak::KECCAK256_DIGEST_SIZE, static_cast<byte>(Keccak::KECCAK256_RATE_SIZE), static_cast<byte>(DEF_PRLDEGREE)) :
		KeccakParams(Keccak::KECCAK256_DIGEST_SIZE, 0x00, 0x00))
{
	Reset();
}

Keccak256::Keccak256(KeccakParams &Params)
	:
	m_dgtState(Params.FanOut() != 0 && Params.FanOut() <= MAX_PRLDEGREE ? Params.FanOut() :
		throw CryptoDigestException(DigestConvert::ToName(Digests::Keccak256), std::string("Constructor"), std::string("The FanOut parameter can not be zero or exceed the maximum of 64!"), ErrorCodes::IllegalOperation)),
	m_msgBuffer(Params.FanOut() * Keccak::KECCAK256_RATE_SIZE),
	m_msgLength(0),
	m_parallelProfile(Keccak::KECCAK256_RATE_SIZE, static_cast<bool>(Params.FanOut() > 1), false, STATE_PRECACHED, false, Params.FanOut()),
	m_treeParams(Params)
{
	Reset();
}

Keccak256::~Keccak256()
{
	m_msgLength = 0;
	IntegerTools::Clear(m_dgtState);
	IntegerTools::Clear(m_msgBuffer);
}

//~~~Accessors~~~//

size_t Keccak256::BlockSize() 
{ 
	return Keccak::KECCAK256_RATE_SIZE; 
}

size_t Keccak256::DigestSize() 
{ 
	return Keccak::KECCAK256_DIGEST_SIZE; 
}

const Digests Keccak256::Enumeral() 
{ 
	return Digests::Keccak256; 
}

const bool Keccak256::IsParallel() 
{
	return m_parallelProfile.IsParallel(); 
}

const std::string Keccak256::Name()
{ 
	std::string name;

	name = DigestConvert::ToName(Enumeral());

	if (m_treeParams.FanOut() > 1)
	{
		name += std::string("-P") + IntegerTools::ToString(m_parallelProfile.ParallelMaxDegree());
	}

	return name;
}

const size_t Keccak256::ParallelBlockSize() 
{ 
	return m_parallelProfile.ParallelBlockSize(); 
}

ParallelOptions &Keccak256::ParallelProfile() 
{ 
	return m_parallelProfile;
}

//~~~Public Functions~~~//

void Keccak256::Compute(const std::vector<byte> &Input, std::vector<byte> &Output)
{
	if (Output.size() < Keccak::KECCAK256_DIGEST_SIZE)
	{
		throw CryptoDigestException(Name(), std::string("Compute"), std::string("The output vector is too small!"), ErrorCodes::InvalidSize);
	}

	Update(Input, 0, Input.size());
	Finalize(Output, 0);
}

void Keccak256::Finalize(std::vector<byte> &Output, size_t OutOffset)
{
	if (Output.size() - OutOffset < Keccak::KECCAK256_DIGEST_SIZE)
	{
		throw CryptoDigestException(Name(), std::string("Finalize"), std::string("The output vector is too small!"), ErrorCodes::InvalidSize);
	}

	size_t bctr;
	size_t boft;
	size_t i;

	if (m_parallelProfile.IsParallel())
	{
		// pad buffer with zeros
		if (m_msgLength < m_msgBuffer.size())
		{
			MemoryTools::Clear(m_msgBuffer, m_msgLength, m_msgBuffer.size() - m_msgLength);
		}

		// process buffer
		if (m_msgLength != 0)
		{
			bctr = 0;

			while (m_msgLength != 0)
			{
				const size_t MSGRMD = (m_msgLength >= Keccak::KECCAK256_RATE_SIZE) ? Keccak::KECCAK256_RATE_SIZE : m_msgLength;
				HashFinal(m_msgBuffer, bctr * Keccak::KECCAK256_RATE_SIZE, MSGRMD, m_dgtState[bctr]);
				m_msgLength -= MSGRMD;
				++bctr;
			}
		}

		// initialize root state
		Keccak256State proot;

		// add state blocks as contiguous message input
		for (i = 0; i < m_dgtState.size(); ++i)
		{
			IntegerTools::LeULL256ToBlock(m_dgtState[i].H, 0, m_msgBuffer, i * Keccak::KECCAK256_DIGEST_SIZE);
			m_msgLength += Keccak::KECCAK256_DIGEST_SIZE;
		}

		// compress full blocks
		boft = 0;

		if (m_msgLength > Keccak::KECCAK256_RATE_SIZE)
		{
			const size_t BLKRMD = m_msgLength - (m_msgLength % Keccak::KECCAK256_RATE_SIZE);

			for (i = 0; i < BLKRMD / Keccak::KECCAK256_RATE_SIZE; ++i)
			{
				Keccak::FastAbsorb(m_msgBuffer, i * Keccak::KECCAK256_RATE_SIZE, Keccak::KECCAK256_RATE_SIZE, proot.H);
				Permute(proot.H);
			}

			m_msgLength -= BLKRMD;
			boft = BLKRMD;
		}

		// finalize and store
		HashFinal(m_msgBuffer, boft, m_msgLength, proot);
		IntegerTools::LeULL256ToBlock(proot.H, 0, Output, OutOffset);
	}
	else
	{
		if (m_msgLength != m_msgBuffer.size())
		{
			MemoryTools::Clear(m_msgBuffer, m_msgLength, m_msgBuffer.size() - m_msgLength);
		}

		HashFinal(m_msgBuffer, 0, m_msgLength, m_dgtState[0]);
		IntegerTools::LeULL256ToBlock(m_dgtState[0].H, 0, Output, OutOffset);
	}

	Reset();
}

void Keccak256::ParallelMaxDegree(size_t Degree)
{
	if (Degree == 0 || Degree % 2 != 0 || Degree > MAX_PRLDEGREE)
	{
		throw CryptoDigestException(Name(), std::string("ParallelMaxDegree"), std::string("Degree setting is invalid!"), ErrorCodes::NotSupported);
	}

	m_parallelProfile.SetMaxDegree(Degree);

	Reset();
}

void Keccak256::Reset()
{
	size_t i;

	MemoryTools::Clear(m_msgBuffer, 0, m_msgBuffer.size());
	m_msgLength = 0;

	for (i = 0; i < m_dgtState.size(); ++i)
	{
		m_dgtState[i].Reset();

		if (m_parallelProfile.IsParallel())
		{
			m_treeParams.NodeOffset() = static_cast<uint>(i);
			Keccak::FastAbsorb(m_treeParams.ToBytes(), 0, Keccak::KECCAK256_RATE_SIZE, m_dgtState[i].H);
			Permute(m_dgtState[i].H);
		}
	}
}

void Keccak256::Update(byte Input)
{
	std::vector<byte> one(1, Input);
	Update(one, 0, 1);
}

void Keccak256::Update(uint Input)
{
	std::vector<byte> tmp(sizeof(uint));
	IntegerTools::Le32ToBytes(Input, tmp, 0);
	Update(tmp, 0, tmp.size());
}

void Keccak256::Update(ulong Input)
{
	std::vector<byte> tmp(sizeof(ulong));
	IntegerTools::Le64ToBytes(Input, tmp, 0);
	Update(tmp, 0, tmp.size());
}

void Keccak256::Update(const std::vector<byte> &Input, size_t InOffset, size_t Length)
{
	CEXASSERT(Input.size() - InOffset >= Length, "The input buffer is too short!");

	if (Length != 0)
	{
		if (m_parallelProfile.IsParallel())
		{
			if (m_msgLength != 0 && Length + m_msgLength >= m_msgBuffer.size())
			{
				// fill buffer
				const size_t RMDLEN = m_msgBuffer.size() - m_msgLength;
				if (RMDLEN != 0)
				{
					MemoryTools::Copy(Input, InOffset, m_msgBuffer, m_msgLength, RMDLEN);
				}

				// empty the message buffer
				ParallelTools::ParallelFor(0, m_parallelProfile.ParallelMaxDegree(), [this, &Input, InOffset](size_t i)
				{
					Keccak::FastAbsorb(m_msgBuffer, i * Keccak::KECCAK256_RATE_SIZE, Keccak::KECCAK256_RATE_SIZE, m_dgtState[i].H);
					Permute(m_dgtState[i].H);
				});

				m_msgLength = 0;
				Length -= RMDLEN;
				InOffset += RMDLEN;
			}

			if (Length >= m_parallelProfile.ParallelBlockSize())
			{
				// calculate working set size
				const size_t PRCLEN = Length - (Length % m_parallelProfile.ParallelBlockSize());

				// process large blocks
				ParallelTools::ParallelFor(0, m_parallelProfile.ParallelMaxDegree(), [this, &Input, InOffset, PRCLEN](size_t i)
				{
					ProcessLeaf(Input, InOffset + (i * Keccak::KECCAK256_RATE_SIZE), m_dgtState[i], PRCLEN);
				});

				Length -= PRCLEN;
				InOffset += PRCLEN;
			}

			if (Length >= m_parallelProfile.ParallelMinimumSize())
			{
				const size_t PRMLEN = Length - (Length % m_parallelProfile.ParallelMinimumSize());

				ParallelTools::ParallelFor(0, m_parallelProfile.ParallelMaxDegree(), [this, &Input, InOffset, PRMLEN](size_t i)
				{
					ProcessLeaf(Input, InOffset + (i * Keccak::KECCAK256_RATE_SIZE), m_dgtState[i], PRMLEN);
				});

				Length -= PRMLEN;
				InOffset += PRMLEN;
			}
		}
		else
		{
			if (m_msgLength != 0 && (m_msgLength + Length >= Keccak::KECCAK256_RATE_SIZE))
			{
				const size_t RMDLEN = Keccak::KECCAK256_RATE_SIZE - m_msgLength;

				if (RMDLEN != 0)
				{
					MemoryTools::Copy(Input, InOffset, m_msgBuffer, m_msgLength, RMDLEN);
				}

				Keccak::FastAbsorb(m_msgBuffer, 0, Keccak::KECCAK256_RATE_SIZE, m_dgtState[0].H);
				Permute(m_dgtState[0].H);
				m_msgLength = 0;
				InOffset += RMDLEN;
				Length -= RMDLEN;
			}

			// sequential loop through blocks
			while (Length >= Keccak::KECCAK256_RATE_SIZE)
			{
				Keccak::FastAbsorb(Input, InOffset, Keccak::KECCAK256_RATE_SIZE, m_dgtState[0].H);
				Permute(m_dgtState[0].H);
				InOffset += Keccak::KECCAK256_RATE_SIZE;
				Length -= Keccak::KECCAK256_RATE_SIZE;
			}
		}

		// store unaligned bytes
		if (Length != 0)
		{
			MemoryTools::Copy(Input, InOffset, m_msgBuffer, m_msgLength, Length);
			m_msgLength += Length;
		}
	}
}

//~~~Private Functions~~~//

void Keccak256::Permute(std::array<ulong, 25> &State)
{
#if defined(CEX_DIGEST_COMPACT)
	Keccak::PermuteR24P1600C(State);
#else
	Keccak::PermuteR24P1600U(State);
#endif
}

void Keccak256::HashFinal(std::vector<byte> &Input, size_t InOffset, size_t Length, Keccak256State &State)
{
	Keccak::AbsorbR24(Input, InOffset, Length, Keccak::KECCAK256_RATE_SIZE, Keccak::KECCAK_SHA3_DOMAIN, State.H);
	Permute(State.H);
}

void Keccak256::ProcessLeaf(const std::vector<byte> &Input, size_t InOffset, Keccak256State &State, ulong Length)
{
	do
	{
		Keccak::FastAbsorb(Input, InOffset, Keccak::KECCAK256_RATE_SIZE, State.H);
		Permute(State.H);
		InOffset += m_parallelProfile.ParallelMinimumSize();
		Length -= m_parallelProfile.ParallelMinimumSize();
	} 
	while (Length > 0);
}

NAMESPACE_DIGESTEND
