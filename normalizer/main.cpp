#define PROGRAM_NAME "crn-normalizer"
#define PROGRAM_VERSION "1.0"

#include <argparse.hpp>
#include "normalizer.hpp"
#include <boost/gil.hpp>
#include <boost/gil/extension/io/png.hpp>
#include "../common/crn.hpp"
#include <fstream>

namespace gil = boost::gil;

#if __has_include(<windows.h>)
#include <windows.h>
std::string UTF8toFileSystemStr(const std::string &srcUTF8)
{
    int lengthUnicode = MultiByteToWideChar(CP_UTF8, 0, srcUTF8.c_str(), srcUTF8.size() + 1, NULL, 0);
    wchar_t bufUnicode[lengthUnicode];
    MultiByteToWideChar(CP_UTF8, 0, srcUTF8.c_str(), srcUTF8.size() + 1, bufUnicode, lengthUnicode);
    int lengthSJis = WideCharToMultiByte(CP_THREAD_ACP, 0, bufUnicode, -1, NULL, 0, NULL, NULL);
    char bufShiftJis[lengthSJis];
    WideCharToMultiByte(CP_THREAD_ACP, 0, bufUnicode, lengthUnicode + 1, bufShiftJis, lengthSJis, NULL, NULL);
    std::string strSJis(bufShiftJis);
    return std::string(bufShiftJis);
}
#else
std::string UTF8toFileSystemStr(const std::string &srcUTF8)
{
    return srcUTF8;
}
#endif

const struct Standard
{
    std::string name;
    char identifier;
    NormalizationParamaters params;
} standards[] = {
    {"scene", 's', {0.5, false}},
    {"icon", 'i', {0.0, false}},
    {"face", 'f', {0.8, true}}};

int main(int argc, char *argv[])
{
    // define arguments

    argparse::ArgumentParser program(PROGRAM_NAME, PROGRAM_VERSION);
    program.add_description("Normalize an image based on a CRN file.");

    program.add_argument("file")
        .help("specify a CRN file (not an image file)");

    program.add_argument("-d", "--dist")
        .default_value(".")
        .help("specify the output directory")
        .metavar("DIST");

    program.add_argument("-n", "--number")
        .default_value("00")
        .help("image identification number (used in output file name)")
        .metavar("NUMBER");

    for (const auto &standard : standards)
        program.add_argument(std::string("-") + standard.identifier, "--" + standard.name)
            .default_value(false)
            .implicit_value(true)
            .help("uses the '" + standard.name + "' standard");

    // parse arguments

    int selectedStandardIndex = -1;
    try
    {
        program.parse_args(argc, argv);

        int i = 0;
        for (const auto &standard : standards)
        {
            if (program[standard.name] == true)
            {
                if (selectedStandardIndex == -1 /* unspecified */)
                    selectedStandardIndex = i;
                else
                    selectedStandardIndex = -2 /* error */;
            }
            ++i;
        }
        if (selectedStandardIndex < 0)
        {
            std::stringstream msg;
            msg << "Error: Specify one of ";
            const int n = sizeof(standards) / sizeof(Standard);
            for (int i = 0; i < n - 1; i++)
                msg << "--" << standards[i].name << ", ";
            msg << "and --" << standards[n - 1].name << ".";
            throw std::runtime_error(msg.str());
        }
    }
    catch (const std::exception &err)
    {
        std::cerr << err.what() << std::endl;
        std::cerr << program;
        std::exit(1);
    }

    // load data from files

    std::string crnPath = program.get<std::string>("file");
    CrnData crnData;
    try
    {
        crnData = CrnData::load(crnPath);
    }
    catch (const CrnData::jsonVersionException &err)
    {
        std::cerr << "Error: cannot read CRN file '" << crnPath << "'" << std::endl;
        std::exit(1);
    }
    catch (...)
    {
        std::exit(1);
    }

    const auto imagePath = crnPath.substr(0, crnPath.size() - 4);
    gil::rgba8_image_t img;
    try
    {
        gil::read_image(imagePath, img, gil::png_tag());
    }
    catch (...)
    {
        std::cerr << "Error: cannot read PNG file '" << imagePath << "'" << std::endl;
        std::exit(1);
    }

    // normalize

    const auto &standard = standards[selectedStandardIndex];
    auto normalizedImage = normalize(standard.params, gil::view(img), crnData);

    // output

    std::stringstream outFileNameS;
    outFileNameS << UTF8toFileSystemStr(crnData.characterName)
                 << "_" << program.get<std::string>("number")
                 << standard.identifier;
    if (!crnData.imageName.empty())
        outFileNameS << "_" << UTF8toFileSystemStr(crnData.imageName);
    outFileNameS << ".png";
    const auto outFilePath = program.get<std::string>("dist") + "/" + outFileNameS.str();

    std::ofstream ofs(outFilePath.c_str(), std::ios_base::binary);
    gil::write_view(ofs, gil::view(normalizedImage), gil::png_tag());

    return 0;
}
