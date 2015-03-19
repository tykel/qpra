#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>

int main(int argc, char *argv[])
{
    FILE *file;
    uint8_t data[4];
    char name[16] = "Khepra test ROM";
    char desc[32] = "Simple ROM testing ops";
    int sz = 68 + 3 + 4 + 3 + 4 + 4;

    file = fopen("test.kpr", "wb");
    // Write magic
    *(char *)&data[0] = 'K';
    *(char *)&data[1] = 'H';
    *(char *)&data[2] = 'P';
    *(char *)&data[3] = 'R';
    fwrite(data, 4, 1, file);
    // Write file size
    *(uint32_t *)data = sz;
    fwrite(data, 4, 1, file);
    // Write (dummy) checksum
    *(uint32_t *)data = 0;
    fwrite(data, 4, 1, file);
    // Write ROM bank numbers
    *(uint32_t *)data = 0x01010101;
    fwrite(data, 4, 1, file);
    // Write reserved bank
    *(uint32_t *)data = 0;
    fwrite(data, 4, 1, file);
    // Write name
    fwrite(name, 16, 1, file);
    // Write description
    fwrite(desc, 32, 1, file);

    // MV a, 1
    data[0] = 0x9a;
    data[1] = 0x40;
    data[2] = 0x01;
    fwrite(data, 3, 1, file);

    // JZ $000e
    data[0] = 0x31;
    data[1] = 0x00;
    data[2] = 0x0e;
    data[3] = 0x00;
    fwrite(data, 4, 1, file);

    // MV a, 0
    data[0] = 0x9a;
    data[1] = 0x40;
    data[2] = 0x00;
    fwrite(data, 3, 1, file);

    // JP $0003
    data[0] = 0x21;
    data[1] = 0x00;
    data[2] = 0x03;
    data[3] = 0x00;
    fwrite(data, 4, 1, file);
    
    // JP $000e
    data[0] = 0x21;
    data[1] = 0x00;
    data[2] = 0x0e;
    data[3] = 0x00;
    fwrite(data, 4, 1, file);

    fclose(file);

    printf("Wrote %d bytes to test.kpr\n", sz);

    return 0;
}
