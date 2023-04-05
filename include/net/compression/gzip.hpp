//#include <boost/iostreams/copy.hpp>
//#include <boost/iostreams/filter/gzip.hpp>
//#include <boost/iostreams/filtering_streambuf.hpp>
//#include <iostream>
//#include <sstream>
//
//#include <boost/archive/iterators/binary_from_base64.hpp>
//#include <boost/archive/iterators/base64_from_binary.hpp>
//#include <boost/archive/iterators/transform_width.hpp>

std::string decode64(std::string const& val)
{
    using namespace boost::archive::iterators;
    return {
        transform_width<binary_from_base64<std::string::const_iterator>, 8, 6>{
            std::begin(val)},
        {std::end(val)},
    };
}

std::string encode64(std::string const& val)
{
    using namespace boost::archive::iterators;
    std::string r{
        base64_from_binary<transform_width<std::string::const_iterator, 6, 8>>{
            std::begin(val)},
        {std::end(val)},
    };
    return r.append((3 - val.size() % 3) % 3, '=');
}

static std::string compress(const std::string& data)
{
    namespace bio = boost::iostreams;
    std::istringstream origin(data);

    bio::filtering_istreambuf in;
    in.push(
        bio::gzip_compressor(bio::gzip_params(bio::gzip::best_compression)));
    in.push(origin);

    std::ostringstream compressed;
    bio::copy(in, compressed);
    return compressed.str();
}

static std::string decompress(const std::string& data)
{
    namespace bio = boost::iostreams;
    std::istringstream compressed(data);

    bio::filtering_istreambuf in;
    in.push(bio::gzip_decompressor());
    in.push(compressed);

    std::ostringstream origin;
    bio::copy(in, origin);
    return origin.str();
}

void testGzip()
{
    auto msg = encode64(compress("my message"));
    std::cout << msg << std::endl;
    std::cout << decompress(decode64(msg)) << std::endl;
}

