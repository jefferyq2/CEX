﻿#ifndef _CEXTEST_AESFIPSTEST_H
#define _CEXTEST_AESFIPSTEST_H

#include "ITest.h"

namespace Test
{
    /// <summary>
	/// Rijndael implementation vector comparison tests.
    /// <para>Test vectors from the NIST standard tests contained in the AES specification document FIPS 197:
    /// <see href="http://csrc.nist.gov/publications/fips/fips197/fips-197.pdf"/> and the 
    /// Monte Carlo AES tests from the Brian Gladman's vector set:
    /// <see href="http://fp.gladman.plus.com/cryptography_technology/rijndael/"/></para>
    /// </summary>
    class AesFipsTest : public ITest
    {
	private:
		const std::string DESCRIPTION = "NIST AES specification FIPS 197 Known Answer Tests.";
		const std::string FAILURE = "FAILURE! ";
		const std::string SUCCESS = "SUCCESS! AES tests have executed succesfully.";

		TestEventHandler m_progressEvent;
        std::vector<std::vector<byte>> m_keys;
        std::vector<std::vector<byte>> m_plainText;
        std::vector<std::vector<byte>> m_cipherText;
		bool m_testNI;

    public:
		/// <summary>
		/// Get: The test description
		/// </summary>
		virtual const std::string Description() { return DESCRIPTION; }

		/// <summary>
		/// Progress return event callback
		/// </summary>
		virtual TestEventHandler &Progress() { return m_progressEvent; }

		/// <summary>
		/// Compares known answer Rijndael vectors for equality (FIPS 197)
		/// </summary>
		explicit AesFipsTest(bool TestNI = false)
			:
			m_testNI(TestNI)
        {
        }

		/// <summary>
		/// Destructor
		/// </summary>
		~AesFipsTest()
		{
		}

		/// <summary>
		/// Start the tests
		/// </summary>
		virtual std::string Run();
        
    private:
		void CompareVector(std::vector<byte> &Key, std::vector<byte> &Input, std::vector<byte> &Output);
		void CompareMonteCarlo(std::vector<byte> &Key, std::vector<byte> &Input, std::vector<byte> &Output);
#if defined(__AVX__)
		void CompareVectorNI(std::vector<byte> &Key, std::vector<byte> &Input, std::vector<byte> &Output);
		void CompareMonteCarloNI(std::vector<byte> &Key, std::vector<byte> &Input, std::vector<byte> &Output);
#endif
		void Initialize();
		void OnProgress(std::string Data);
    };
}

#endif
