#include <gtest/gtest.h>
#include <net/uri.hpp>

using namespace std::literals;

namespace net { namespace testing {
	using ::testing::TestWithParam;
	using ::testing::ValuesIn;

	struct UriCannonicalHrefTest {
		std::string base;
		std::string href;
		std::string expected;
	};

	struct UriCannonicalHexTest {
		std::string url;
		std::string expected;
	};

	std::ostream& operator<<(std::ostream& o, const UriCannonicalHrefTest& param)
	{
		return o << "\"" << param.base << "\" + \"" << param.href << "\"";
	}

	std::ostream& operator<<(std::ostream& o, const UriCannonicalHexTest& param)
	{
		return o << "\"" << param.url << "\" -> \"" << param.expected << "\"";
	}

	class UriCannonicalHref : public TestWithParam<UriCannonicalHrefTest> {
	};

	class UriCannonicalHex : public TestWithParam<UriCannonicalHexTest> {
	};

	TEST_P(UriCannonicalHref, HrefAttr)
	{
		auto param = GetParam();
		auto result = Uri::canonical(param.href, param.base).string();
		ASSERT_EQ(param.expected, result);
	}

	TEST_P(UriCannonicalHex, URLDecode)
	{
		auto param = GetParam();
		auto result = Uri::canonical(param.url, { }).string();
		ASSERT_EQ(param.expected, result);
	}

	static const UriCannonicalHrefTest uri_canonical_empties[] = {
		{ "http://example.com",               "",                         "http://example.com/" },
		{ "http://example.com/",              "",                         "http://example.com/" },
		{ "http://example.com",               "/",                        "http://example.com/" },
		{ "http://example.com/",              "/",                        "http://example.com/" },
		{ "http://example.com",               "http://example.net",       "http://example.net/" },
		{ "http://example.com/A/B/some.file", "",                         "http://example.com/A/B/some.file/" },
		{ "http://example.com/A/B/",          "",                         "http://example.com/A/B/" },
	};

	static const UriCannonicalHrefTest uri_canonical_roots[] = {
		{ "http://example.com/A/B/some.file", "/C/other.file",            "http://example.com/C/other.file" },
		{ "http://example.com/A/B/",          "/C/other.file",            "http://example.com/C/other.file" },
	};

	static const UriCannonicalHrefTest uri_canonical_here[] = {
		{ "http://example.com/A/B/some.file", "C/other.file",             "http://example.com/A/B/some.file/C/other.file" },
		{ "http://example.com/A/B/",          "C/other.file",             "http://example.com/A/B/C/other.file" },
	};

	static const UriCannonicalHrefTest uri_canonical_up[] = {
		{ "http://example.com/A/B/some.file", "../C/other.file",          "http://example.com/A/B/C/other.file" },
		{ "http://example.com/A/B/some.file", "../../C/other.file",       "http://example.com/A/C/other.file" },
		{ "http://example.com/A/B/some.file", "../../../C/other.file",    "http://example.com/C/other.file" },
		{ "http://example.com/A/B/some.file", "../../../../C/other.file", "http://example.com/C/other.file" },
		{ "http://example.com/A/B/",          "../C/other.file",          "http://example.com/A/C/other.file" },
		{ "http://example.com/A/B/",          "../../C/other.file",       "http://example.com/C/other.file" },
		{ "http://example.com/A/B/",          "../../../C/other.file",    "http://example.com/C/other.file" },
	};

	static const UriCannonicalHexTest uri_canonical_decode_auth[] = {
		{ "http://example.com/",                               "http://example.com/" },
		{ "http://%45xample.com/",                             "http://example.com/" },
		{ "http://example%2Fslash.com/",                       "" },
		{ "HTTP://EXAMPLE.COM/",                               "http://example.com/" },
		{ "http://127.0.0.1/",                                 "http://127.0.0.1/" },
		{ "http://[::1]/",                                     "http://[::1]/" },
		{ "http://example.com:/",                              "http://example.com/" },
		{ "http://example.com:80/",                            "http://example.com/" },
		{ "https://example.com:443/",                          "https://example.com/" },
		{ "ftp://example.com:21/",                             "ftp://example.com/" },
		{ "http://example.com:8080/",                          "http://example.com:8080/" },
		{ "http://example.com:blah/",                          "" },
	};

	static const UriCannonicalHexTest uri_canonical_decode_path[] = {
		{ "http://example.com/FileName.ext",                   "http://example.com/FileName.ext" },
		{ "http://EXAMPLE.COM/FileName.ext",                   "http://example.com/FileName.ext" },
		{ "http://example.com/Dir/subdir%2Ffake/FileName.ext", "http://example.com/Dir/subdir%2Ffake/FileName.ext" },
		{ "http://example.com/Dir/subdir%2ffake/FileName.ext", "http://example.com/Dir/subdir%2Ffake/FileName.ext" },
		{ "http://example.com/Dir/subdir/Fil%65Name.ext",      "http://example.com/Dir/subdir/FileName.ext" },
		{ "http://example.com/Dir/subdir/File Name.ext",       "http://example.com/Dir/subdir/File%20Name.ext" },
		{ "http://example.com/A/B/./some.file",                "http://example.com/A/B/some.file" },
		{ "http://example.com/A/B/./",                         "http://example.com/A/B/" },
		{ "http://example.com/A/B/.",                          "http://example.com/A/B/" },
		{ "http://example.com/A/B/../some.file",               "http://example.com/A/some.file" },
		{ "http://example.com/A/B/../",                        "http://example.com/A/" },
		{ "http://example.com/A/B/..",                         "http://example.com/A/" },
		{ "http://example.com/A/B/../../../some.file",         "http://example.com/some.file" },
		{ "http://example.com/A/B/../../../C/../",             "http://example.com/" },
		{ "http://example.com/A/B/../../.%2e/C/..",            "http://example.com/" },
		{ "http://example.com/%7Euser",                        "http://example.com/~user" }
	};

	static const UriCannonicalHexTest uri_canonical_decode_relpath[] = {
		{ "A/B/../../../some.file",                             "../some.file" },
		{ "/A/B/../../../some.file",                            "/some.file" },
		{ "A/B/../../some.file",                                "some.file" },
		{ "A/../../../some.file",                               "../../some.file" },
	};

	static const UriCannonicalHexTest uri_canonical_decode_query[] = {
		{ "?name=value",                                        "?name=value" },
	};

#define URI_CANONICAL(Name, Domain, arr) \
	INSTANTIATE_TEST_CASE_P(Name, UriCannonical ## Domain, ValuesIn(arr))

	URI_CANONICAL(Empties, Href, uri_canonical_empties);
	URI_CANONICAL(Roots, Href, uri_canonical_roots);
	URI_CANONICAL(Here, Href, uri_canonical_here);
	URI_CANONICAL(Up, Href, uri_canonical_up);
	URI_CANONICAL(Authority, Hex, uri_canonical_decode_auth);
	URI_CANONICAL(Path, Hex, uri_canonical_decode_path);
	URI_CANONICAL(RelPath, Hex, uri_canonical_decode_relpath);
	URI_CANONICAL(Query, Hex, uri_canonical_decode_query);
}}
