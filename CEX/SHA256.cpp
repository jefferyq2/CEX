#include "SHA256.h"
#include "IntUtils.h"
#include "MemUtils.h"
#include "ParallelUtils.h"
#include "SHA256Compress.h"

NAMESPACE_DIGEST

const std::string SHA256::CLASS_NAME("SHA256");

// *** Properties *** //

size_t SHA256::BlockSize() 
{ 
	return BLOCK_SIZE; 
}

size_t SHA256::DigestSize() 
{ 
	return DIGEST_SIZE;
}

const Digests SHA256::Enumeral() 
{
	return Digests::SHA256; 
}

const bool SHA256::IsParallel()
{ 
	return m_parallelProfile.IsParallel(); 
}

const std::string SHA256::Name()
{ 
	if (m_parallelProfile.IsParallel())
		return CLASS_NAME + "-P" + Utility::IntUtils::ToString(m_parallelProfile.ParallelMaxDegree());
	else
		return CLASS_NAME;
}

const size_t SHA256::ParallelBlockSize()
{ 
	return m_parallelProfile.ParallelBlockSize(); 
}

ParallelOptions &SHA256::ParallelProfile() 
{ 
	return m_parallelProfile; 
}

//~~~Constructor~~~//

SHA256::SHA256(bool Parallel)
	:
	m_treeParams(DIGEST_SIZE, static_cast<uint>(BLOCK_SIZE), DEF_PRLDEGREE),
	m_isDestroyed(false),
	m_msgBuffer(Parallel ? DEF_PRLDEGREE * BLOCK_SIZE : BLOCK_SIZE),
	m_msgLength(0),
	m_parallelProfile(BLOCK_SIZE, false, STATE_PRECACHED, false, DEF_PRLDEGREE),
	m_dgtState(Parallel ? DEF_PRLDEGREE : 1)
{
	if (m_parallelProfile.IsParallel())
		m_parallelProfile.IsParallel() = Parallel;

	Reset();
}

SHA256::SHA256(SHA2Params &Params)
	:
	m_treeParams(Params),
	m_dgtState(1),
	m_isDestroyed(false),
	m_msgBuffer(BLOCK_SIZE),
	m_msgLength(0),
	m_parallelProfile(BLOCK_SIZE, false, STATE_PRECACHED, false, m_treeParams.FanOut())
{
	if (m_treeParams.FanOut() > 1)
	{
		m_dgtState.resize(m_treeParams.FanOut());
		m_msgBuffer.resize(m_treeParams.FanOut() * BLOCK_SIZE);
	}
	else if (m_parallelProfile.IsParallel())
	{
		m_parallelProfile.IsParallel() = false;
	}

	Reset();
}

SHA256::~SHA256()
{
	Destroy();
}

//~~~Public Functions~~~//

void SHA256::Compute(const std::vector<byte> &Input, std::vector<byte> &Output)
{
	Output.resize(DIGEST_SIZE);
	Update(Input, 0, Input.size());
	Finalize(Output, 0);
}

void SHA256::Destroy()
{
	if (!m_isDestroyed)
	{
		m_isDestroyed = true;
		m_msgLength = 0;

		try
		{
			for (size_t i = 0; i < m_dgtState.size(); ++i)
				m_dgtState[i].Reset();

			Utility::IntUtils::ClearVector(m_msgBuffer);
			Utility::IntUtils::ClearVector(m_dgtState);
		}
		catch (std::exception& ex)
		{
			throw CryptoDigestException("SHA256:Destroy", "Could not clear all variables!", std::string(ex.what()));
		}
	}
}

size_t SHA256::Finalize(std::vector<byte> &Output, const size_t OutOffset)
{
	CEXASSERT(Output.size() - OutOffset >= DIGEST_SIZE, "The Output buffer is too short!");

	if (m_parallelProfile.IsParallel())
	{
		// pad buffer with zeros
		if (m_msgLength < m_msgBuffer.size())
			Utility::MemUtils::Clear<byte>(m_msgBuffer, m_msgLength, m_msgBuffer.size() - m_msgLength);

		// process buffer
		if (m_msgLength != 0)
		{
			size_t blkCtr = 0;

			while (m_msgLength != 0)
			{
				const size_t MSGRMD = (m_msgLength >= BLOCK_SIZE) ? BLOCK_SIZE : m_msgLength;
				HashFinal(m_msgBuffer, blkCtr * BLOCK_SIZE, MSGRMD, m_dgtState[blkCtr]);
				m_msgLength -= MSGRMD;
				++blkCtr;
			}
		}

		// initialize root state
		SHA256State rootState;
		rootState.Reset();

		// add state blocks as contiguous message input
		for (size_t i = 0; i < m_dgtState.size(); ++i)
		{
			Utility::IntUtils::BeUL256ToBlock(m_dgtState[i].H, 0, m_msgBuffer, i * BLOCK_SIZE);
			m_msgLength += DIGEST_SIZE;
		}

		// compress full blocks
		size_t blkOff = 0;
		if (m_msgLength > BLOCK_SIZE)
		{
			const size_t BLKRMD = m_msgLength - (m_msgLength % BLOCK_SIZE);

			for (size_t i = 0; i < BLKRMD / BLOCK_SIZE; ++i)
				Compress(m_msgBuffer, i * BLOCK_SIZE, rootState);

			m_msgLength -= BLKRMD;
			blkOff = BLKRMD;
		}

		// finalize and store
		HashFinal(m_msgBuffer, blkOff, m_msgLength, rootState);
		Utility::IntUtils::BeUL256ToBlock(rootState.H, 0, Output, OutOffset);
	}
	else
	{
		// finalize and store
		HashFinal(m_msgBuffer, 0, m_msgLength, m_dgtState[0]);
		Utility::IntUtils::BeUL256ToBlock(m_dgtState[0].H, 0, Output, OutOffset);
	}

	Reset();

	return DIGEST_SIZE;
}

void SHA256::ParallelMaxDegree(size_t Degree)
{
	if (Degree == 0)
		throw CryptoDigestException("SHA256:ParallelMaxDegree", "Parallel degree can not be zero!");
	if (Degree > 254)
		throw CryptoDigestException("SHA256:ParallelMaxDegree", "Parallel degree can not exceed 254!");
	if (Degree % 2 != 0)
		throw CryptoDigestException("SHA256:ParallelMaxDegree", "Parallel degree must be an even number!");

	m_parallelProfile.SetMaxDegree(Degree);
	m_dgtState.clear();
	m_dgtState.resize(Degree);
	m_msgBuffer.clear();
	m_msgBuffer.resize(Degree * BLOCK_SIZE);

	Reset();
}

void SHA256::Reset()
{
	m_msgLength = 0;
	Utility::MemUtils::Clear<byte>(m_msgBuffer, 0, m_msgBuffer.size());

	for (size_t i = 0; i < m_dgtState.size(); ++i)
	{
		m_dgtState[i].Reset();

		if (m_parallelProfile.IsParallel())
		{
			m_treeParams.NodeOffset() = static_cast<uint>(i);
			Compress(m_treeParams.ToBytes(), 0, m_dgtState[i]);
		}
	}
}

void SHA256::Update(byte Input)
{
	std::vector<byte> inp(1, Input);
	Update(inp, 0, 1);
}

void SHA256::Update(const std::vector<byte> &Input, size_t InOffset, size_t Length)
{
	CEXASSERT(Input.size() - InOffset >= Length, "The Output buffer is too short!");

	if (Length == 0)
		return;

	if (m_parallelProfile.IsParallel())
	{
		if (m_msgLength != 0 && Length + m_msgLength >= m_msgBuffer.size())
		{
			// fill buffer
			const size_t RMDLEN = m_msgBuffer.size() - m_msgLength;
			if (RMDLEN != 0)
				Utility::MemUtils::Copy<byte>(Input, InOffset, m_msgBuffer, m_msgLength, RMDLEN);

			// empty the message buffer
			Utility::ParallelUtils::ParallelFor(0, m_parallelProfile.ParallelMaxDegree(), [this, &Input, InOffset](size_t i)
			{
				Compress(m_msgBuffer, i * BLOCK_SIZE, m_dgtState[i]);
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
			Utility::ParallelUtils::ParallelFor(0, m_parallelProfile.ParallelMaxDegree(), [this, &Input, InOffset, PRCLEN](size_t i)
			{
				ProcessLeaf(Input, InOffset + (i * BLOCK_SIZE), m_dgtState[i], PRCLEN);
			});

			Length -= PRCLEN;
			InOffset += PRCLEN;
		}

		if (Length >= m_parallelProfile.ParallelMinimumSize())
		{
			const size_t PRMLEN = Length - (Length % m_parallelProfile.ParallelMinimumSize());

			Utility::ParallelUtils::ParallelFor(0, m_parallelProfile.ParallelMaxDegree(), [this, &Input, InOffset, PRMLEN](size_t i)
			{
				ProcessLeaf(Input, InOffset + (i * BLOCK_SIZE), m_dgtState[i], PRMLEN);
			});

			Length -= PRMLEN;
			InOffset += PRMLEN;
		}
	}
	else
	{
		if (m_msgLength != 0 && (m_msgLength + Length >= BLOCK_SIZE))
		{
			const size_t RMDLEN = BLOCK_SIZE - m_msgLength;
			if (RMDLEN != 0)
				Utility::MemUtils::Copy<byte>(Input, InOffset, m_msgBuffer, m_msgLength, RMDLEN);

			Compress(m_msgBuffer, 0, m_dgtState[0]);
			m_msgLength = 0;
			InOffset += RMDLEN;
			Length -= RMDLEN;
		}

		// sequential loop through blocks
		while (Length > BLOCK_SIZE)
		{
			Compress(Input, InOffset, m_dgtState[0]);
			InOffset += BLOCK_SIZE;
			Length -= BLOCK_SIZE;
		}
	}

	// store unaligned bytes
	if (Length != 0)
	{
		Utility::MemUtils::Copy<byte>(Input, InOffset, m_msgBuffer, m_msgLength, Length);
		m_msgLength += Length;
	}
}

//~~~Private Functions~~~//

void SHA256::HashFinal(std::vector<byte> &Input, size_t InOffset, size_t Length, SHA256State &State)
{
	State.T += Length;
	ulong bitLen = (State.T << 3);

	if (Length == BLOCK_SIZE)
	{
		SHA256Compress::Compress64(Input, InOffset, State);
		Length = 0;
	}

	Input[InOffset + Length] = (byte)128;
	++Length;

	// padding
	if (Length < BLOCK_SIZE)
		Utility::MemUtils::Clear<byte>(Input, InOffset + Length, BLOCK_SIZE - Length);

	if (Length > 56)
	{
		SHA256Compress::Compress64(Input, InOffset, State);
		Utility::MemUtils::Clear<byte>(Input, 0, BLOCK_SIZE);
	}

	// finalize state with counter and last compression
	Utility::IntUtils::Be32ToBytes((uint)((ulong)bitLen >> 32), Input, InOffset + 56);
	Utility::IntUtils::Be32ToBytes((uint)((ulong)bitLen), Input, InOffset + 60);
	SHA256Compress::Compress64(Input, InOffset, State);
}

void SHA256::Compress(const std::vector<byte> &Input, size_t InOffset, SHA256State &State)
{
	if (m_parallelProfile.HasSHA2())
		SHA256Compress::Compress64W(Input, InOffset, State);
	else
		SHA256Compress::Compress64(Input, InOffset, State);
}

void SHA256::ProcessLeaf(const std::vector<byte> &Input, size_t InOffset, SHA256State &State, ulong Length)
{
	do
	{
		Compress(Input, InOffset, State);
		InOffset += m_parallelProfile.ParallelMinimumSize();
		Length -= m_parallelProfile.ParallelMinimumSize();
	} 
	while (Length > 0);
}

NAMESPACE_DIGESTEND