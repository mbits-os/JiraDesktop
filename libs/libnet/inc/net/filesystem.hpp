#pragma once

#if HAVE(TS_FILESYSTEM)
#	include <experimental/filesystem>
#else // !HAVE(TS_FILESYSTEM)
#	include <boost/filesystem.hpp>
#endif // HAVE(TS_FILESYSTEM)

namespace fs {

#if HAVE(TS_FILESYSTEM)
using namespace std::experimental::filesystem;
using std::error_code;
#else // !HAVE(TS_FILESYSTEM)
using namespace boost::filesystem;
using boost::system::error_code;

inline directory_iterator begin(const directory_iterator& iter)
{
	return iter;
}

inline directory_iterator end(const directory_iterator&)
{
	return directory_iterator { };
}
#endif // HAVE(TS_FILESYSTEM)
}
