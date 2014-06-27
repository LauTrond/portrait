//Coding.cc
//这是对Coding.hh的实现

#include "sybie/datain/Coding.hh"

#include <cstdint>
#include <utility>
#include <memory>
#include <iostream>

#include "sybie/common/Streaming.hh"

namespace sybie {
namespace datain {

namespace {

//assert( BytesPerUnit_Bin * BitsPerByte_Bin
//     == BytesPerUnit_Txt * BitsPerByte_Txt )
//assert( BitsPerByte_Bin > BitsPerByte_Txt )
enum {BytesPerUnit_Bin = 3, BytesPerUnit_Txt = 4};
enum {BitsPerByte_Bin = 8, BitsPerByte_Txt = 6};

const char* GetEncodingMap()
{
    static char _EncodingMap[] = //64 = (1<<BitsPerByte_Txt)
        "0123456789" //10
        "ABCDEFGHIJKLMNOPQRSTUVWXYZ" //26
        "abcdefghijklmnopqrstuvwxyz" //26
        "+-"; //2
    return _EncodingMap;
}

const char* MakeDecodingMap()
{
    static char _DecodingMap[256];
    memset(_DecodingMap, -1, sizeof(_DecodingMap));
    const char* EncodingMap = GetEncodingMap();

    for (int i = 0 ; EncodingMap[i] != '\0' ; i++)
        _DecodingMap[(int)EncodingMap[i]] = (char)i;

    return _DecodingMap;
}

const char* GetDecodingMap()
{
    static const char* _DecodingMap = MakeDecodingMap(); //run only once;
    return _DecodingMap;
}

const int* MakeBinSizeToTxtSizeMap()
{
    static int _BinSizeToTxtSizeMap[BytesPerUnit_Bin + 1]; //0,2,3,4
    for (int i = 0 ; i <= BytesPerUnit_Bin ; i++)
    {
        int bits = i * BitsPerByte_Bin;
        _BinSizeToTxtSizeMap[i] = bits / BitsPerByte_Txt
                                + (bits % BitsPerByte_Txt > 0);
    }
    return _BinSizeToTxtSizeMap;
}

const int* GetBinSizeToTxtSizeMap()
{
    static const int* _BinSizeToTxtSizeMap
                      = MakeBinSizeToTxtSizeMap(); //run only once;
    return _BinSizeToTxtSizeMap;
}

const int* MakeTxtSizeToBinSizeMap()
{
    static int _TxtSizeToBinSizeMap[BytesPerUnit_Txt + 1]; //0,0,1,2,3
    for (int i = 0 ; i <= BytesPerUnit_Txt ; i++)
        _TxtSizeToBinSizeMap[i] = (i * BitsPerByte_Txt) / BitsPerByte_Bin;
    return _TxtSizeToBinSizeMap;
}

const int* GetTxtSizeToBinSizeMap()
{
    static const int* _TxtSizeToBinSizeMap
                      = MakeTxtSizeToBinSizeMap(); //run only once;
    return _TxtSizeToBinSizeMap;
}

typedef int32_t UnitValType;

} //namespace

size_t Encode(std::istream& bin, std::ostream& txt)
{
    const char* EncodingMap = GetEncodingMap();
    const int* BinSizeToTxtSizeMap = GetBinSizeToTxtSizeMap();
    size_t sum_bytes = 0;
    //UnitValType check_sum = 0;

    while (!bin.eof())
    {
        char bin_unit[BytesPerUnit_Bin];
        memset(bin_unit, 0, BytesPerUnit_Bin);
        char txt_unit[BytesPerUnit_Txt];

        //在文件末尾，可能读出0个字节，这不会导致问题，下个循环退出。
        bin.read(bin_unit, BytesPerUnit_Bin);
        UnitValType unit_val = 0;

        for (int i = 0 ; i < BytesPerUnit_Bin ; i++)
            unit_val |= (UnitValType)(unsigned char)bin_unit[i]
                        << (BitsPerByte_Bin * i);
        //check_sum += unit_val;
        const UnitValType mask = ((UnitValType)1<<BitsPerByte_Txt) - 1;
        for (int i = 0 ; i < BytesPerUnit_Txt ; i++)
            txt_unit[i] = EncodingMap[(unit_val >> (BitsPerByte_Txt * i)) & mask];

        int bytes = BinSizeToTxtSizeMap[bin.gcount()];
        txt.write(txt_unit, bytes);
        sum_bytes += bytes;
        if (txt.fail())
            throw std::runtime_error("Encode: Failed write data to output stream.");
    }

    //std::cout<<"Checksum:"<<check_sum<<std::endl;
    return sum_bytes;
}

size_t Decode(std::istream& txt, std::ostream& bin)
{
    const char* DecodingMap = GetDecodingMap();
    const int* TxtSizeToBinSizeMap = GetTxtSizeToBinSizeMap();
    size_t sum_bytes = 0;
    //UnitValType check_sum = 0;

    while (!txt.eof())
    {
        char txt_unit[BytesPerUnit_Txt];
        memset(txt_unit, '0', BytesPerUnit_Txt);
        char bin_unit[BytesPerUnit_Bin];

        //在文件末尾，可能读出0个字节，这不会导致问题，下个循环退出。
        txt.read(txt_unit, BytesPerUnit_Txt);
        UnitValType unit_val = 0;

        for (int i = 0 ; i < BytesPerUnit_Txt ; i++)
        {
            UnitValType decode = DecodingMap[(int)txt_unit[i]];
            if (decode < 0)
                throw std::runtime_error("Decode: Invalid source data.");
            unit_val |= decode << (BitsPerByte_Txt * i);
        }
        //check_sum += unit_val;
        const UnitValType mask = ((UnitValType)1<<BitsPerByte_Bin) - 1;
        for (int i = 0 ; i < BytesPerUnit_Bin ; i++)
            bin_unit[i] = (char)(unsigned char)((unit_val >> (BitsPerByte_Bin * i)) & mask);

        int bytes = TxtSizeToBinSizeMap[txt.gcount()];
        bin.write(bin_unit, bytes);
        sum_bytes += bytes;
        if (bin.fail())
            throw std::runtime_error("Decode: Failed write data to output stream.");
    }

    //std::cout<<"Checksum:"<<check_sum<<std::endl;
    return sum_bytes;
}

std::string Encode(const std::string& bin)
{
    size_t result_size = GetEncodeResultSize(bin.size());
    std::string txt(result_size, '\0');
    common::ByteArrayStream bin_stream(const_cast<std::string&>(bin));
    common::ByteArrayStream txt_stream(txt);
    Encode(bin_stream, txt_stream);
    return txt;
}

std::string Decode(const std::string& txt)
{
    size_t result_size = GetDecodeResultSize(txt.size());
    std::string bin(result_size, '\0');
    common::ByteArrayStream txt_stream(const_cast<std::string&>(txt));
    common::ByteArrayStream bin_stream(bin);
    Decode(txt_stream, bin_stream);
    return bin;
}

size_t GetEncodeResultSize(const size_t bin_size)
{
    return bin_size / BytesPerUnit_Bin * BytesPerUnit_Txt
         + GetBinSizeToTxtSizeMap()[bin_size % BytesPerUnit_Bin];
}

size_t GetDecodeResultSize(const size_t txt_size)
{
    return txt_size / BytesPerUnit_Txt * BytesPerUnit_Bin
         + GetTxtSizeToBinSizeMap()[txt_size % BytesPerUnit_Txt];
}

} //namespace datain
} //namespace sybie
