#ifdef _MSC_VER
#define _CRT_SECURE_NO_WARNINGS
#endif
#include <iostream>
#include <fstream>
#include <vector>
#include <algorithm>
#include <sys/stat.h>

using namespace std;

#define code_value_bits 16

int fget_bit(unsigned char* input_bit, unsigned int* bit_len, FILE* input, unsigned int* useless_bit)
{
    if ((*bit_len) == 0)
    {
        (*input_bit) = fgetc(input);
        if (feof(input))
        {
            (*useless_bit)++;
            if ((*useless_bit) > 14)
            {
                throw invalid_argument("decompression failed");
            }
        }
        (*bit_len) = 8;
    }
    int result = (*input_bit) & 1;
    (*input_bit) >>= 1;
    (*bit_len)--;
    return result;
}

void decoder(const char* input_name = "encode.txt",
    const char* output_name = "decode.txt") {
    unsigned int* alfabet = new unsigned int[256];
    for (int i = 0; i < 256; i++)
    {
        alfabet[i] = 0;
    }
    FILE* in = fopen(input_name, "rb");
    if (in == nullptr)
    {
        throw invalid_argument("missing file");
    }

    unsigned char col = 0;
    unsigned int col_letters = 0;
    col = fgetc(in);
    if (!feof(in)) {
        col_letters = (unsigned int)col;
    }

    unsigned char character = 0;

    for (int i = 0; i < col_letters; i++) {
        character = fgetc(in);
        if (!feof(in)) {
            fread(reinterpret_cast<char*>(&alfabet[character]),
                sizeof(unsigned short), 1, in);
        }
        else {
            throw invalid_argument("decopression failed");
        }
    }

    vector<pair<char, unsigned int>> vec;
    for (int i = 0; i < 256; i++)
    {
        if (alfabet[i] != 0)
        {
            vec.push_back(make_pair(static_cast<char>(i), alfabet[i]));
        }
    }

    sort(vec.begin(), vec.end(), [](const pair<char, unsigned int>& left, const pair<char, unsigned int>& right)
        {
            if (left.second != right.second)
            {
                return left.second >= right.second;
            }
            return left.first < right.first;
        });
    for (auto pair : vec)
    {
        cout << pair.first << " " << pair.second << endl;
    }

    unsigned short* ranges = new unsigned short[vec.size() + 2];
    ranges[0] = 0;
    ranges[1] = 1;
    for (int i = 0; i < vec.size(); i++) {
        unsigned int b = vec[i].second;
        for (int j = 0; j < i; j++) {
            b += vec[j].second;
        }
        ranges[i + 2] = b;
    }

    unsigned int valueLow = 0;
    unsigned int valueHigh = ((static_cast<unsigned int>(1) << code_value_bits) - 1);
    unsigned int divider = ranges[vec.size() + 1];
    unsigned int first_qtr = (valueHigh + 1) / 4;
    unsigned int half = first_qtr * 2;
    unsigned int third_qtr = first_qtr * 3;

    unsigned int bit_len = 0;
    unsigned char input_bit = 0;
    unsigned int useless_bit = 0;
    unsigned short valueCode = 0;
    int tmp = 0;

    FILE* out = fopen(output_name, "wb +");
    if (out == nullptr)
    {
        throw invalid_argument("missing file");
    }

    for (int i = 1; i <= 16; i++)
    {
        tmp = fget_bit(&input_bit, &bit_len, in, &useless_bit);
        valueCode = 2 * valueCode + tmp;
    }
    unsigned int diff = valueHigh - valueLow + 1;
    for (;;)
    {
        unsigned int freq = static_cast<unsigned int>(((static_cast<unsigned int>(valueCode) - valueLow + 1) * divider - 1) / diff);

        int j;

        for (j = 1; ranges[j] <= freq; j++) {}
        valueHigh = valueLow + ranges[j] * diff / divider - 1;
        valueLow = valueLow + ranges[j - 1] * diff / divider;

        for (;;)
        {
            if (valueHigh < half) {}

            else if (valueLow >= half)
            {
                valueLow -= half;
                valueHigh -= half;
                valueCode -= half;
            }
            else if ((valueLow >= first_qtr) && (valueHigh < third_qtr))
            {
                valueLow -= first_qtr;
                valueHigh -= first_qtr;
                valueCode -= first_qtr;
            }
            else
            {
                break;
            }

            valueLow += valueLow;
            valueHigh += valueHigh + 1;
            tmp = 0;
            tmp = fget_bit(&input_bit, &bit_len, in, &useless_bit);
            valueCode += valueCode + tmp;
        }

        if (j == 1)
        {
            break;
        }

        fputc(vec[j - 2].first, out);
        diff = valueHigh - valueLow + 1;
    }
    cout << "success\n";
    fclose(in);
    fclose(out);
}

unsigned int checker(const char* before_name = "text.txt",
    const char* after_name = "decode.txt") {
    unsigned int same = 0;
    FILE* before = fopen(before_name, "r");
    FILE* after = fopen(after_name, "r");

    unsigned char after_l = 0;
    unsigned char before_l = 0;
    while (!feof(after) && !feof(before))
    {
        after_l = fgetc(after);
        before_l = fgetc(before);
        if (!feof(after) && !feof(before))
        {
            if (after_l != before_l)
            {
                same++;
            }
        }
    }

    while (!feof(after))
    {
        after_l = fgetc(after);
        if (!feof(after))
        {
            same++;
        }
    }

    while (!feof(before))
    {
        before_l = fgetc(before);
        if (!feof(before))
        {
            same++;
        }
    }
    fclose(after);
    fclose(before);
    return same;
}

int main()
{
    decoder();
    if (!checker())
    {
        cout << "match" << endl;
    }
    else
    {
        cout << "wrong match" << endl;
    }
}