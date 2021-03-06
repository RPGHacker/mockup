#include "cpu.h"
#include "memory.h"


inline u8 r65816_op_readpc(r65816_cpu_t* cpu) {
    //Hack: The following assumes, that we only execute from ROM. It gives significant speed boost though.
    //return cpu->rom->data[((cpu->regs.pc.b & 0x7F) << 15) | (cpu->regs.pc.w++ & 0x7FFF)];
    //This should be the right approach.
    return r65816_cpu_read(cpu, cpu->regs.pc.d++);
}
inline u8 r65816_op_readstack(r65816_cpu_t* cpu) {
    cpu->regs.e ? cpu->regs.s.l++ : cpu->regs.s.w++;
    return r65816_cpu_read(cpu, cpu->regs.s.w);
}

inline u8 r65816_op_readstackn(r65816_cpu_t* cpu) {
    return r65816_cpu_read(cpu, ++cpu->regs.s.w);
}

inline u8 r65816_op_readaddr(r65816_cpu_t* cpu, u32 addr) {
    return r65816_cpu_read(cpu, addr & 0xffff);
}

inline u8 r65816_op_readlong(r65816_cpu_t* cpu, u32 addr) {
    return r65816_cpu_read(cpu, addr & 0xffffff);
}

inline u8 r65816_op_readdbr(r65816_cpu_t* cpu, u32 addr) {
    return r65816_cpu_read(cpu, ((cpu->regs.db << 16) + addr) & 0xffffff);
}

inline u8 r65816_op_readpbr(r65816_cpu_t* cpu, u32 addr) {
    return r65816_cpu_read(cpu, (cpu->regs.pc.b << 16) + (addr & 0xffff));
}

inline u8 r65816_op_readdp(r65816_cpu_t* cpu, u32 addr) {
    if(cpu->regs.e && cpu->regs.d.l == 0x00) {
        return r65816_cpu_read(cpu, (cpu->regs.d.w & 0xff00) + ((cpu->regs.d.w + (addr & 0xffff)) & 0xff));
    } else {
        return r65816_cpu_read(cpu, (cpu->regs.d.w + (addr & 0xffff)) & 0xffff);
    }
}

inline u8 r65816_op_readsp(r65816_cpu_t* cpu, u32 addr) {
    return r65816_cpu_read(cpu, (cpu->regs.s.w + (addr & 0xffff)) & 0xffff);
}

inline void r65816_op_writestack(r65816_cpu_t* cpu, u8 data) {
    r65816_cpu_write(cpu, cpu->regs.s.w, data);
    cpu->regs.e ? cpu->regs.s.l-- : cpu->regs.s.w--;
}

inline void r65816_op_writestackn(r65816_cpu_t* cpu, u8 data) {
    r65816_cpu_write(cpu, cpu->regs.s.w--, data);
}

inline void r65816_op_writeaddr(r65816_cpu_t* cpu, u32 addr, u8 data) {
    r65816_cpu_write(cpu, addr & 0xffff, data);
}

inline void r65816_op_writelong(r65816_cpu_t* cpu, u32 addr, u8 data) {
    r65816_cpu_write(cpu, addr & 0xffffff, data);
}

inline void r65816_op_writedbr(r65816_cpu_t* cpu, u32 addr, u8 data) {
    r65816_cpu_write(cpu, ((cpu->regs.db << 16) + addr) & 0xffffff, data);
}

inline void r65816_op_writepbr(r65816_cpu_t* cpu, u32 addr, u8 data) {
    r65816_cpu_write(cpu, (cpu->regs.pc.b << 16) + (addr & 0xffff), data);
}

inline void r65816_op_writedp(r65816_cpu_t* cpu, u32 addr, u8 data) {
    if(cpu->regs.e && cpu->regs.d.l == 0x00) {
        r65816_cpu_write(cpu, (cpu->regs.d.w & 0xff00) + ((cpu->regs.d.w + (addr & 0xffff)) & 0xff), data);
    } else {
        r65816_cpu_write(cpu, (cpu->regs.d.w + (addr & 0xffff)) & 0xffff, data);
    }
}

inline void r65816_op_writesp(r65816_cpu_t* cpu, u32 addr, u8 data) {
    r65816_cpu_write(cpu, (cpu->regs.s.w + (addr & 0xffff)) & 0xffff, data);
}
