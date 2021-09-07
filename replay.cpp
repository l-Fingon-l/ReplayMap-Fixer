#include "replay.h"

// Thanks fingon for MapNames list!
unordered_set<string> MapNames = {
    "(2)amazonia.w3x",
    "(2)autumnleaves.w3x",
    "(2)concealedhill.w3x",
    "(2)dalaran.w3x",
    "(2)echoisles.w3x",
    "(2)lastrefuge.w3x",
    "(2)northernisles.w3x",
    "(2)swampedtemple.w3x",
    "(2)terenasstand_lv.w3x",
    "(2)tidehunters.w3x",
    "(3)nomadisles.w3x",
    "(4)turtlerock.w3x",
    "(4)twistedmeadows.w3x"
};

const int block_size = 8192, add_space = 500, compression_level = 1;

////////////////////////////////////////////////////////////////////////////

uint16_t xor16(uint32_t x)
{
    return (uint16_t)(x ^ (x >> 16));
}

void BlockHeader::compute_checkshum(BYTE* block)
{
    check1 = xor16(crc32(0L, (BYTE*)this, sizeof(BlockHeader)));
    check2 = xor16(crc32(0L, block, c_size));
}

bool W3GHeader::read(FILE* f)
{
    memset(this, 0, sizeof(W3GHeader));
    if (fread(this, 1, 48, f) != 48)
        return false;
    if (memcmp(intro, "Warcraft III recorded game", 26))
        return false;
    if (header_v == 1)
    {
        if (fread(&ident, 1, 4, f) != 4) return false;
        if (fread(&patch_v, 1, 4, f) != 4) return false;
        if (fread(&build_v, 1, 2, f) != 2) return false;
        if (fread(&flags, 1, 2, f) != 2) return false;
        if (fread(&length, 1, 4, f) != 4) return false;
        if (fread(&checksum, 1, 4, f) != 4) return false;
    }
    else return false;
    return true;
}

void W3GHeader::compute_checksum()
{
    checksum = 0;
    checksum = crc32(0L, (BYTE*)this, header_size);
}

void real_u_size(BYTE* dataUncompressed, int& size)
{
    int amount_of_nulls = 0, pos = size - 1;
    while (dataUncompressed[pos--] == 0) amount_of_nulls++;
    size -= amount_of_nulls;
}

bool read_replay(FILE* read, W3GHeader& hdr, BYTE* body_c)
{
    int pos = ftell(read);
    fseek(read, 0, SEEK_END);
    if (ftell(read) < hdr.c_size)
    {
        cout << "The data in replay is corrupted!\n";
        return false;
    }
    fseek(read, pos, SEEK_SET);
    fread(body_c, 1, hdr.c_size - hdr.header_size, read);

    return true;
}

bool write_replay(const char* name, W3GHeader& hdr, BYTE* body_c)
{
    ofstream out{ name, out.binary };
    if (!out.is_open())
    {
        cout << "Replay " << name << " was not created. Writing error!\n";
        cout << "Make sure a folder named 'Converted' is present next to the 'Replays' one.\n";
        return false;
    }
    hdr.compute_checksum();
    out.write((char*)&hdr, hdr.header_size);
    out.write((char*)body_c, hdr.c_size - hdr.header_size);
    out.close();

    return true;
}

bool decompress_block(BlockHeader& bhdr, BYTE* dataCompressed, BYTE* dataUncompressed)
{
    z_stream strm;

    // allocate inflate state 
    strm.zalloc = Z_NULL;
    strm.zfree = Z_NULL;
    strm.opaque = Z_NULL;
    strm.avail_in = bhdr.c_size;
    strm.next_in = Z_NULL;
    if (inflateInit(&strm) != Z_OK)
        return false;

    // decompress until deflate stream ends or end of file 
    strm.next_in = dataCompressed;
    strm.avail_out = bhdr.u_size;
    strm.next_out = dataUncompressed;

    if (inflate(&strm, Z_SYNC_FLUSH) != Z_OK)
        return false;
    (void)inflateEnd(&strm);
    return true;
}

bool compress_block(BlockHeader& bhdr, BYTE* dataCompressed, BYTE* dataUncompressed)
{
    int ret, flush;
    z_stream strm;
    memset(&bhdr, 0, sizeof(BlockHeader));
    bhdr.u_size = block_size;

    /* allocate deflate state */
    strm.zalloc = Z_NULL;
    strm.zfree = Z_NULL;
    strm.opaque = Z_NULL;
    ret = deflateInit(&strm, compression_level);
    if (ret != Z_OK)
        return false;

    /* compress until end of file */
    strm.avail_in = bhdr.u_size;
    flush = Z_SYNC_FLUSH;
    strm.next_in = dataUncompressed;
    strm.avail_out = CHUNK;
    strm.next_out = dataCompressed;
    deflate(&strm, flush);
    bhdr.c_size = CHUNK - strm.avail_out;

    /* set the block header checksums*/
    bhdr.compute_checkshum(dataCompressed);

    /* clean up and return */
    (void)deflateEnd(&strm);
    return true;
}

void getEncodedString(BYTE* buffer, BYTE* block, int& pos, int& length)
{
    pos = 6;
    while (block[pos++] != 0); // reading the name of the host
    pos += block[pos++];
    while (block[pos++] != 0); // reading the Game Name
    pos++;
    length = -pos;
    while (block[pos++] != 0); // reading the Encoded String
    length += pos;
    memcpy(buffer, block + pos - length, length);
}

void decodeEncStr(BYTE* EncodedString, int& size)
{
    uint8_t mask;
    int pos = 0;
    int dpos = 0;
    BYTE* Buffer = new BYTE[size];

    while (EncodedString[pos] != 0)
    {
        if (pos % 8 != 0)
        {
            Buffer[dpos++] = EncodedString[pos++] & (0xFFFE | ((mask & (0x1 << (pos % 8))) >> (pos % 8)));
        }
        else mask = EncodedString[pos++];
    }

    for (int i(0); i < dpos; i++)
    {
        EncodedString[i] = Buffer[i];
    }
    delete[] Buffer;
    size = dpos;
}

bool modifyEncodedString(BYTE* EncStr, int& length, const string& maps_location)
{
    decodeEncStr(EncStr, length);
    uint32_t crc_time = 0xFFFFFFFF;
    memcpy(EncStr + 9, &crc_time, 4); // setting map-crc
    int pos = 13;
    while (EncStr[pos++] != 0); // getting to the end of Map Name
    int name_length = pos - 13;
    string name = "";
    do pos--;
    while ((EncStr[pos] != 0x2F) & (EncStr[pos] != 0x5C)); // getting to the start of Map Name
    pos++;
    while (EncStr[pos] != 0) name += EncStr[pos++];

    std::transform(name.begin(), name.end(), name.begin(),
        [](unsigned char c) { return std::tolower(c); });
    if (MapNames.find(name) == MapNames.end()) // in case the map wasn't found
    {
        cout << "This replay doesn't need a conversion!\n";
        return false;
    }

    name = maps_location + '/' + name;
    int namesize = name.size() + 1;
    int old_length = length;
    length += (namesize - name_length); // updated length
    BYTE NewEncStr[add_space];
    BYTE* NewStrPos = NewEncStr;
    memcpy(NewStrPos, EncStr, 13);
    NewStrPos += 13;                          // get to the beginning of the new map name
    memcpy(NewStrPos, &name[0], name.size());
    NewEncStr[13 + name.size()] = 0;
    NewStrPos += namesize;                    // get to the beginning of the host name
    memcpy(NewStrPos, EncStr + 13 + name_length, old_length - 13 - name_length);

    memcpy(EncStr, NewEncStr, length);
    return true;
}

void encodeEncStr(BYTE* EncStr, int& length)
{
    uint8_t Mask = 1;
    int old_length = length;
    int new_length = old_length + old_length / 7 + ((old_length % 7) > 0);
    BYTE result[add_space];

    for (int i = 0, j = 1; i < length; i++)
    {
        if ((EncStr[i] % 2) == 0)
            result[i + j] = EncStr[i] + 1;
        else
        {
            result[i + j] = EncStr[i];
            Mask |= 1 << ((i % 7) + 1);
        }

        if (i % 7 == 6 || i == length - 1)
        {
            result[i + j - (i % 7) - 1] = Mask;
            Mask = 1;
            j++;
        }
    }
    length = new_length;
    for (int i(0); i < length; i++)
        EncStr[i] = result[i];

    EncStr[length++] = 0;
}

bool decompress_and_modify(BYTE* dataCompressed, BYTE* dataUncompressed, W3GHeader& hdr, const string& maps_location)
{
    int u_size_before = hdr.u_size;
    hdr.u_size += block_size;
    BYTE BlockUncompressed[CHUNK];
    int blocks_left = hdr.blocks, pos_c = 0, pos_u = 0;
    BlockHeader bhdr;

    // decompress the first block

    memcpy(&bhdr, dataCompressed + pos_c, sizeof(BlockHeader));
    if (!decompress_block(bhdr, dataCompressed + pos_c + sizeof(BlockHeader), BlockUncompressed))
    {
        cout << "A decompression mistake. The data inside replay is corrupted!\n";
        return false;
    }
    memcpy(dataUncompressed, BlockUncompressed, block_size);
    pos_c += bhdr.c_size + sizeof(BlockHeader);
    blocks_left--;

    // modify the first block

    int EncStrLength = 0;
    BYTE* EncodedString = new BYTE[add_space];
    getEncodedString(EncodedString, dataUncompressed, pos_u, EncStrLength);
    int length_before = EncStrLength;
    if (!modifyEncodedString(EncodedString, EncStrLength, maps_location))
    {
        cout << "";
        return false;
    }
    encodeEncStr(EncodedString, EncStrLength);

    int shift = EncStrLength - length_before;
    memcpy(dataUncompressed + pos_u + shift, dataUncompressed + pos_u, block_size - pos_u);
    pos_u -= length_before;
    memcpy(dataUncompressed + pos_u, EncodedString, EncStrLength);
    pos_u = block_size + shift;
    delete[] EncodedString;

    // other blocks

    while (blocks_left--)
    {
        memcpy(&bhdr, dataCompressed + pos_c, sizeof(BlockHeader));
        if (!decompress_block(bhdr, dataCompressed + pos_c + sizeof(BlockHeader), BlockUncompressed))
        {
            cout << "Decompression mistake. The data inside replay is corrupted!\n";
            return false;
        }
        memcpy(dataUncompressed + pos_u, BlockUncompressed, block_size);
        pos_c += bhdr.c_size + sizeof(BlockHeader);
        pos_u += bhdr.u_size;
    }

    // trunkating 0s to fit in the same amount of blocks

    while (!dataUncompressed[pos_u - 1] && shift-- > 0) pos_u--; // it's important not to update it inside the while check

    // updating the padding with 0s, amount of blocks and the u_size

    hdr.blocks = pos_u / block_size + ((pos_u % block_size) > 0);
    hdr.u_size = block_size * hdr.blocks;
    memset(dataUncompressed + pos_u, 0, hdr.u_size - pos_u);
    hdr.u_size = u_size_before + shift;

    return true;
}

bool compress(BYTE* dataCompressed, BYTE* dataUncompressed, W3GHeader& hdr)
{
    BYTE BlockCompressed[CHUNK];
    BlockHeader bhdr;
    int blocks_left = hdr.blocks, pos_u = 0, pos_c = 0;
    while (blocks_left--)
    {
        if (!compress_block(bhdr, BlockCompressed, dataUncompressed + pos_u))
        {
            cout << "Compression mistake!\n";
            return false;
        }
        memcpy(dataCompressed + pos_c, &bhdr, sizeof(BlockHeader));
        pos_c += sizeof(BlockHeader);
        memcpy(dataCompressed + pos_c, BlockCompressed, bhdr.c_size);
        pos_c += bhdr.c_size;
        pos_u += block_size;
    }
    hdr.c_size = pos_c + hdr.header_size;

    return true;
}

bool repack_replay(const char* replay_name, const char* converted_name, uint16_t build, int& amount_success, const string& maps_location)
{
    W3GHeader replay_header;
#pragma region ReadReplay
    FILE* read;
    if (fopen_s(&read, replay_name, "rb"))
    {
        cout << "Replay " << replay_name << " was not found!\n";
        return false;
    }
    if (!replay_header.read(read))
    {
        cout << "This is either not a Warcraft III replay file or it is corrupted!\n";
        return false;
    }

    replay_header.build_v = build;

    BYTE* replay_c = new BYTE[replay_header.c_size + add_space];
    if (!read_replay(read, replay_header, replay_c))
    {
        cout << "A mistake in reading " << replay_name << ".\n";
        return false;
    }
    fclose(read);
#pragma endregion

#pragma region ModifyReplay
    if (true)
    {
        BYTE* replay_u = new BYTE[replay_header.u_size + block_size];
        if (!decompress_and_modify(replay_c, replay_u, replay_header, maps_location))
        {
            cout << "=>" << replay_name << "\n";
            return false;
        }

        if (!compress(replay_c, replay_u, replay_header))
        {
            cout << "A mistake of compression in the " << replay_name << ".\n";
            return false;
        }

        delete[] replay_u;
    }
#pragma endregion

    if (!write_replay(converted_name, replay_header, replay_c))
    {
        cout << "A mistake of creation the " << converted_name << ".\n";
        cout << "Check out if there is no such file in existance in the folder already.\n";
        return false;
    }

    delete[] replay_c;
    amount_success++;
    return true;
}
