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

int indexOFsymbol(char symbol, vector<pair<char, unsigned int>> vec)
{
    for (int i = 0; i < vec.size(); i++)
    {
        if (symbol == vec[i].first)
        {
            return i + 2;
        }
    }

    return -1;
}

void fput_bit(unsigned int bit, unsigned int* bit_len, unsigned char* file_bit, FILE* output)
{
    (*file_bit) = (*file_bit) >> 1;
    if (bit) (*file_bit) |= (1 << 7);

    (*bit_len)--;

    if ((*bit_len) == 0)
    {
        fputc((*file_bit), output);
        (*bit_len) = 8;
    }
}

void followBits(unsigned int bit, unsigned int* follow_bits, unsigned int* bit_len, unsigned char* write_bit, FILE* output_file)
{
    fput_bit(bit, bit_len, write_bit, output_file);

    for (; *follow_bits > 0; (*follow_bits)--)
    {
        fput_bit(!bit, bit_len, write_bit, output_file);
    }
}

void coder(const char* input_text = "text.txt", const char* output_text = "encode.txt")
{
    uint64_t* alfabet = new uint64_t[256];
    for (int i = 0; i < 256; i++) {
        alfabet[i] = 0;
    }

    FILE* in = fopen(input_text, "rb");
    if (in == nullptr) {
        throw invalid_argument("missing file");
    }

    unsigned char character = 0;

    while (!feof(in))
    {
        character = fgetc(in);
        if (!feof(in))
        {
            alfabet[character]++;
        }
    }

    fclose(in);

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
    for (int i = 0; i < vec.size(); i++)
    {
        unsigned int b = vec[i].second;
        for (int j = 0; j < i; j++)
        {
            b += vec[j].second;
        }
        ranges[i + 2] = b;
    }

    in = fopen(input_text, "rb");
    FILE* out = fopen(output_text, "wb +");

    char lettersCount = vec.size();
    fputc(lettersCount, out);

    for (int i = 0; i < 256; i++)
    {
        if (alfabet[i] != 0)
        {
            fputc(static_cast<char>(i), out);
            fwrite(reinterpret_cast<const char*>(&alfabet[i]),
                sizeof(unsigned short), 1, out);
        }
    }

    unsigned int valueLow = 0;
    unsigned int valueHigh = ((static_cast<unsigned int>(1) << code_value_bits) - 1);

    unsigned int divider = ranges[vec.size() + 1];
    unsigned int diff = valueHigh - valueLow + 1;
    unsigned int first_qtr = (valueHigh + 1) / 4;
    unsigned int half = first_qtr * 2;
    unsigned int third_qtr = first_qtr * 3;

    unsigned int follow_bits = 0;
    unsigned int bit_len = 8;
    unsigned char write_bit = 0;

    int j = 0;

    while (!feof(in))
    {
        character = fgetc(in);

        if (!feof(in))
        {
            j = indexOFsymbol(character, vec);

            valueHigh = valueLow + ranges[j] * diff / divider - 1;
            valueLow = valueLow + ranges[j - 1] * diff / divider;

            for (;;)
            {
                if (valueHigh < half)
                {
                    followBits(0, &follow_bits,
                        &bit_len, &write_bit, out);
                }
                else if (valueLow >= half)
                {
                    followBits(1, &follow_bits,
                        &bit_len, &write_bit, out);
                    valueLow -= half;
                    valueHigh -= half;
                }
                else if ((valueLow >= first_qtr) && (valueHigh < third_qtr))
                {
                    follow_bits++;
                    valueLow -= first_qtr;
                    valueHigh -= first_qtr;
                }
                else
                {
                    break;
                }

                valueLow += valueLow;
                valueHigh += valueHigh + 1;
            }
        }

        else
        {
            valueHigh = valueLow + ranges[1] * diff / divider - 1;
            valueLow = valueLow + ranges[0] * diff / divider;

            for (;;)
            {
                if (valueHigh < half)
                {
                    followBits(0, &follow_bits,
                        &bit_len, &write_bit, out);
                }
                else if (valueLow >= half)
                {
                    followBits(1, &follow_bits,
                        &bit_len, &write_bit, out);
                    valueLow -= half;
                    valueHigh -= half;
                }
                else if ((valueLow >= first_qtr) && (valueHigh < third_qtr))
                {
                    follow_bits++;
                    valueLow -= first_qtr;
                    valueHigh -= first_qtr;
                }
                else
                {
                    break;
                }

                valueLow += valueLow;
                valueHigh += valueHigh + 1;
            }

            follow_bits++;

            if (valueLow < first_qtr)
            {
                followBits(0, &follow_bits,
                    &bit_len, &write_bit, out);
            }
            else
            {
                followBits(1, &follow_bits,
                    &bit_len, &write_bit, out);
            }

            write_bit >>= bit_len;
            fputc(write_bit, out);
        }
        diff = valueHigh - valueLow + 1;
    }
    cout << "success\n";
    fclose(in);
    fclose(out);
}

float compressRatio(const char* input_name = "text.txt", const char* output_name = "encode.txt") {
    uint64_t file_fullsize = 0;
    uint64_t compress_size = 0;

    struct stat sb {};
    struct stat se {};

    if (!stat(input_name, &sb)) {
        file_fullsize = sb.st_size;
    }
    else {
        perror("STAT");
    }
    if (!stat(output_name, &se)) {
        compress_size = se.st_size;
    }
    else {
        perror("STAT");
    }

    return (compress_size + 0.0) / file_fullsize;
}

int main() {
    coder();
    cout << "compress ratio: " << compressRatio() << endl;
}