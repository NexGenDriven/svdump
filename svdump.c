/*
 * svdump.c, dump the SonicVibes synth DSP memory spaces (For 86Box reverse engineering)
 *
 * Run it on a real S3 SonicVibes card (Not with experimental 86Box emulation), from MS-DOS without any drivers loaded
 *
 * Build (Open Watcom):               wcl svdump.c
 * Build (Turbo C or Borland C):      bcc svdump.c
 *
 * Usage:  svdump <enhanced-base-hex> [outprefix]
 *         e.g. svdump B400 svdsp
 *
 *          By: nxdcaptain (NexGenDriven)
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <conio.h>

static unsigned base;

static void
selreg(unsigned reg)
{
    outp(base + 8, reg & 0xff);
    outp(base + 9, reg >> 8);
}

static void
wreg(unsigned reg, unsigned val) /* High byte first, as S3CVXD (Win9X) does */
{
    selreg(reg);
    outp(base + 0xb, val >> 8);
    outp(base + 0xa, val & 0xff);
}

static unsigned
rreg(unsigned reg)
{
    unsigned hi, lo;

    selreg(reg);
    hi = inp(base + 0xb);
    lo = inp(base + 0xa);
    return (hi << 8) | lo;
}

static int
readword(unsigned addr, unsigned ctrl, unsigned *out)
{
    long t;

    wreg(0, addr);
    wreg(3, ctrl | 0x0002);
    for (t = 0; t < 100000L; t++) {
        selreg(3);
        if (!(inp(base + 0xa) & 0x02))
            break;
    }
    if (t >= 100000L)
        return 0;
    *out = rreg(1);
    return 1;
}

int
main(int argc, char **argv)
{
    static const unsigned spacebit[3] = { 0x0200, 0x0100, 0x0080 };
    const char *prefix = (argc > 2) ? argv[2] : "svdsp";
    unsigned    space, variant, addr, w;
    unsigned    saved3;
    int         ok;

    if (argc < 2) {
        printf("usage: svdump <enhanced-base-hex> [prefix]\n");
        return 1;
    }
    base = (unsigned) strtoul(argv[1], NULL, 16);
    printf("enhanced base %04X, revision reg 0x15 -> ", base);
    outp(base + 4, 0x15);
    printf("%02X\n", inp(base + 5));

    saved3 = rreg(3);
    for (space = 0; space < 3; space++) {
        for (variant = 0; variant < 2; variant++) {
            char  name[64];
            FILE *f;
            unsigned ctrl = spacebit[space] | (variant ? 0x0040 : 0x0020);

            sprintf(name, "%s_s%u_%c.bin", prefix, space, variant ? 'b' : 'a');
            f = fopen(name, "wb");
            if (!f) {
                printf("cannot create %s\n", name);
                return 1;
            }
            ok = 1;
            for (addr = 0; ok && addr < 0xfffeU; addr += 2) {
                if (!readword(addr, ctrl, &w)) {
                    printf("%s: strobe timeout at %04X, stopping this file\n", name, addr);
                    ok = 0;
                    break;
                }
                fputc(w & 0xff, f);
                fputc(w >> 8, f);
            }
            fclose(f);
            printf("wrote %s\n", name);
        }
    }
    wreg(3, saved3 & ~0x0006);
    return 0;
}
