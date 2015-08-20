#include <gtest/gtest.h>
#include <net/uri.hpp>

using namespace std::literals;

namespace net { namespace testing {
	using ::testing::TestWithParam;
	using ::testing::Range;

	struct UriCannonicalTest {
		std::string base;
		std::string href;
		std::string expected;
	};

	std::ostream& operator<<(std::ostream& o, const UriCannonicalTest& param)
	{
		return o << "\"" << param.base << "\" + \"" << param.href << "\"";
	}

	std::ostream& operator<<(std::ostream& o, const UriCannonicalTest* pparam)
	{
		if (pparam)
			return o << *pparam;
		return o << "nullptr";
	}

	class UriCannonical : public TestWithParam<const UriCannonicalTest*> {
	};

	TEST_P(UriCannonical, work)
	{
		auto param = *GetParam();
		auto result = Uri::canonical(param.href, param.base).string();
		ASSERT_EQ(param.expected, result);
	}

	static const UriCannonicalTest uri_canonical_empties[] = {
		{ "http://example.com",               "",                         "http://example.com/" },
		{ "http://example.com/",              "",                         "http://example.com/" },
		{ "http://example.com",               "/",                        "http://example.com/" },
		{ "http://example.com/",              "/",                        "http://example.com/" },
		{ "http://example.com",               "http://example.net",       "http://example.net" },
		{ "http://example.com/A/B/some.file", "",                         "http://example.com/A/B/some.file/" },
		{ "http://example.com/A/B/",          "",                         "http://example.com/A/B/" },
	};

	static const UriCannonicalTest uri_canonical_roots[] = {
		{ "http://example.com/A/B/some.file", "/C/other.file",            "http://example.com/C/other.file" },
		{ "http://example.com/A/B/",          "/C/other.file",            "http://example.com/C/other.file" },
	};

	static const UriCannonicalTest uri_canonical_here[] = {
		{ "http://example.com/A/B/some.file", "C/other.file",             "http://example.com/A/B/some.file/C/other.file" },
		{ "http://example.com/A/B/",          "C/other.file",             "http://example.com/A/B/C/other.file" },
	};

	static const UriCannonicalTest uri_canonical_up[] = {
		{ "http://example.com/A/B/some.file", "../C/other.file",          "http://example.com/A/B/C/other.file" },
		{ "http://example.com/A/B/some.file", "../../C/other.file",       "http://example.com/A/C/other.file" },
		{ "http://example.com/A/B/some.file", "../../../C/other.file",    "http://example.com/C/other.file" },
		{ "http://example.com/A/B/some.file", "../../../../C/other.file", "http://example.com/C/other.file" },
		{ "http://example.com/A/B/",          "../C/other.file",          "http://example.com/A/C/other.file" },
		{ "http://example.com/A/B/",          "../../C/other.file",       "http://example.com/C/other.file" },
		{ "http://example.com/A/B/",          "../../../C/other.file",    "http://example.com/C/other.file" },
	};

#define URI_CANONICAL(Name, arr) \
	INSTANTIATE_TEST_CASE_P(Name, UriCannonical, Range(std::begin(arr), std::end(arr)))

	URI_CANONICAL(Empties, uri_canonical_empties);
	URI_CANONICAL(Roots, uri_canonical_roots);
	URI_CANONICAL(Here, uri_canonical_here);
	URI_CANONICAL(Up, uri_canonical_up);
}}
