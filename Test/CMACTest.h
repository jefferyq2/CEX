#ifndef _CEXTEST_CMACTEST_H
#define _CEXTEST_CMACTEST_H

#include "ITest.h"

namespace Test
{
	/// <summary>
	/// HMAC implementation vector comparison tests.
	/// <para>Using vectors from Rfc 4493:
	/// <see href="http://tools.ietf.org/html/rfc4493"/></para>
	/// </summary>
	class CMACTest : public ITest
	{
	private:
		const std::string DESCRIPTION = "CMAC Known Answer Test Vectors for 128/192/256 bit Keys.";
		const std::string FAILURE = "FAILURE! ";
		const std::string SUCCESS = "SUCCESS! All CMAC tests have executed succesfully.";

		TestEventHandler m_progressEvent;
		std::vector<std::vector<byte>> m_expected;
		std::vector<std::vector<byte>> m_input;
		std::vector<std::vector<byte>> m_keys;

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
		/// Compares known answer CMAC vectors for equality
		/// </summary>
		CMACTest()
		{
		}

		/// <summary>
		/// Destructor
		/// </summary>
		~CMACTest()
		{
		}

		/// <summary>
		/// Start the tests
		/// </summary>
		virtual std::string Run();

	private:
		void CompareAccess(std::vector<byte> &Key);
		void CompareVector(std::vector<byte> &Key, std::vector<byte> &Input, std::vector<byte> &Expected);
		void Initialize();
		void OnProgress(std::string Data);
	};
}

#endif