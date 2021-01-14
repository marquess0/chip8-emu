#include <stdint.h>
#include <unistd.h>
#include <SDL2/SDL.h>
#include <stdio.h>
#include <stdlib.h>
uint16_t opcode;
uint8_t mem[4096];
uint8_t hexa[5 * 16] = { 
  0xF0, 0x90, 0x90, 0x90, 0xF0, // 0
  0x20, 0x60, 0x20, 0x20, 0x70, // 1
  0xF0, 0x10, 0xF0, 0x80, 0xF0, // 2
  0xF0, 0x10, 0xF0, 0x10, 0xF0, // 3
  0x90, 0x90, 0xF0, 0x10, 0x10, // 4
  0xF0, 0x80, 0xF0, 0x10, 0xF0, // 5
  0xF0, 0x80, 0xF0, 0x90, 0xF0, // 6
  0xF0, 0x10, 0x20, 0x40, 0x40, // 7
  0xF0, 0x90, 0xF0, 0x90, 0xF0, // 8
  0xF0, 0x90, 0xF0, 0x10, 0xF0, // 9
  0xF0, 0x90, 0xF0, 0x90, 0x90, // A
  0xE0, 0x90, 0xE0, 0x90, 0xE0, // B
  0xF0, 0x80, 0x80, 0x80, 0xF0, // C
  0xE0, 0x90, 0x90, 0x90, 0xE0, // D
  0xF0, 0x80, 0xF0, 0x80, 0xF0, // E
  0xF0, 0x80, 0xF0, 0x80, 0x80,  // F
};
uint8_t reg[16];
uint16_t i;
uint16_t pc = 0x200;
uint8_t screen[8][32];
int pitch = 0;
uint8_t delay, sound;
uint16_t sp;
uint8_t key[16] = {0};
// 0 is nothing, 1 is press, 2 is held, 3 is release
clock_t t;
int main(int argc, char *argv[]) {
	if (argc > 1) {
		FILE* f = fopen(argv[1], "rb");
		fread(mem + 0x200,4096-0x200,1,f);
		fclose(f);
		for (int ind = 0; ind < 5 * 16; ind++) {
			mem[ind] = hexa[ind];
		}
	}
	SDL_Init(SDL_INIT_VIDEO);
	SDL_Window * win = SDL_CreateWindow("Chip8_Emu", SDL_WINDOWPOS_UNDEFINED, SDL_WINDOWPOS_UNDEFINED, 1024, 512, SDL_WINDOW_OPENGL);
	SDL_Renderer* ren = SDL_CreateRenderer(win, -1, SDL_RENDERER_ACCELERATED);
	t = clock();
	SDL_SetRenderTarget(ren, NULL);
	srand((unsigned) time(&t));
	for (;;) {
		printf("Instruction at %x : %x: ", pc, (mem[pc] << 8) | mem[pc+1]);
		opcode = mem[pc] << 8 | mem[pc+1];
		switch(opcode) {
			case 0x00C0:
			case 0x00C1:
			case 0x00C2:
			case 0x00C3:
			case 0x00C4:
			case 0x00C5:
			case 0x00C6:
			case 0x00C7:
			case 0x00C8:
			case 0x00C9:
			case 0x00CA:
			case 0x00CB:
			case 0x00CC:
			case 0x00CD:
			case 0x00CE:
			case 0x00CF:
				//scroll down by x lines (x is last nibble)
				//Super only, not implemented
				break;
			case 0x00E0:
				//clear the screen
				printf("Clear Screen\n");
				SDL_SetRenderDrawColor(ren, 0, 0, 0, SDL_ALPHA_OPAQUE);
				SDL_RenderClear(ren);
				break;
			case 0x00EE:
				//Return from Subroutine Call
				printf("Return from subroutine\n");
				pc = stack[sp];
				sp--;
				break;
			case 0x00FB:
				//scroll right 4 pixels
				//Super only, not implemented
				break;
			case 0x00FC:
				//scroll left 4 pixels
				//Super only, not implemented
				break;
			case 0x00FE:
				//disable extended screen mode
				//Super only
				break;
			case 0x00FF:
				//enable extended screen mode (128 x 64)
				//Super only
				break;
			default:
				if (opcode >> 12 == 1) {
					//Jump to address xxx (last 12 bits of opcode)
					printf("jump to %x\n", opcode & 0xfff);
					pc = opcode & 0xfff;
					pc-=2;
				} else if (opcode >> 12 == 2) {
					//Jump to subroutine at address xxx (last 12 bits of opcode)
					//16 levels maximum (I think of stack)
					printf("jump to subroutine at %x\n", opcode & 0xfff);
					stack[++sp] = pc;
					pc = opcode & 0xfff;
					pc-=2;
				} else if (opcode >> 12 == 3) {
					//skip if reg r == xx (rxx last 12 bits)
					printf("skip if register %x == %d\n", ((opcode >> 8) & 0xf), (opcode & 0xff));
					if (reg[((opcode >> 8) & 0xf)] == (opcode & 0xff)) {
						printf("skipped\n");
						pc+=2;
					}
				} else if (opcode >> 12 == 4) {
					//skip if reg r <> xx (rxx last 12 bits)
					printf("skip if register %x != %d\n", ((opcode >> 8) & 0xf), (opcode & 0xff));
					if (reg[((opcode >> 8) & 0xf)] != (opcode & 0xff)) {
						printf("skipped\n");
						pc+=2;
					}
				} else if (opcode >> 12 == 5 && (opcode & 0xf) == 0) {
					//skip if register r = register y (ry middle 8 bits)
					printf("skip if register %x == register %x\n", ((opcode >> 8) &0xf), ((opcode >> 4) & 0xf));
					if (reg[((opcode >> 8) & 0xf)] == reg[((opcode >> 4) & 0xf)]) {
						printf("skipped\n");
						pc+=2;
					}
				} else if (opcode >> 12 == 6) {
					//move constant xx to register r (rxx last 12 bits)
					printf("move constant %d into register %x\n", ((opcode & 0xff)), ((opcode >> 8) & 0xf));
					reg[((opcode >> 8) & 0xf)] = (opcode & 0xff);
				} else if (opcode >> 12 == 7) {
					//add constant to register r (rxx last 12 bits)
					//no carry generated
					printf("add constant %d to register %x with no carry\n", (opcode & 0xff), ((opcode >> 8) & 0xf));
					reg[((opcode >> 8) & 0xf)] += (opcode & 0xff);
				} else if (opcode >> 12 == 8) {
					if ((opcode & 0xf) == 0) {
						//move register y into r (ry middle 8)
						printf("move register %x into register %x\n", ((opcode >> 4) & 0xf), ((opcode >> 8) & 0xf));
						reg[((opcode >> 8) & 0xf)] = reg[((opcode >> 4) & 0xf)];
					} else if ((opcode & 0xf) == 1) {
						//or register y into register r (ry middle 8)
						printf("OR register %x into register %x\n", ((opcode >> 4) & 0xf), ((opcode >> 8) & 0xf));
						reg[((opcode >> 8) & 0xf)] |= reg[((opcode >> 4) & 0xf)];
					} else if ((opcode & 0xf) == 2) {
						//and y into r (ry Middle 8)
						printf("AND register %x into register %x\n", ((opcode >> 4) & 0xf), ((opcode >> 8) & 0xf));
						reg[((opcode >> 8) & 0xf)] &= reg[((opcode >> 4) & 0xf)];
					} else if ((opcode & 0xf) == 3) {
						//xor y into r (ry middle 8)
						printf("XOR register %x into register %x\n", ((opcode >> 4) & 0xf), ((opcode >> 8) & 0xf));
						reg[((opcode >> 8) & 0xf)] ^= reg[((opcode >> 4) & 0xf)];
					} else if ((opcode & 0xf) == 4) {
						//add y into r, carry in f (ry in middle 8)
						printf("add register %x into register %x, carry in register f\n", ((opcode >> 4) & 0xf), ((opcode >> 8) & 0xf));
						reg[((opcode >> 8) & 0xf)] += reg[((opcode >> 4) & 0xf)];
						if (reg[((opcode >> 8) & 0xf)] < reg[((opcode >> 4) & 0xf)]) {
							reg[15] = 1;
							printf("carry bit set\n");
						} else {
							reg[15] = 0;
						}
					} else if ((opcode & 0xf) == 5) {
						//sub y from r, borrow in f (ry in middle 8
						// vf set to 1 if borrows)
						printf("subtract register %x from register %x, borrow in register f\n", ((opcode >> 4) & 0xf), ((opcode >> 8) & 0xf));
						if (reg[((opcode >> 8) & 0xf)] > reg[((opcode >> 4) & 0xf)]) {
							reg[15] = 1;
							printf("borrow bit set\n");
						} else {
							reg[15] = 0;
						}
						reg[((opcode >> 8) & 0xf)] -= reg[((opcode >> 4) & 0xf)];
					} else if ((opcode & 0xf) == 6) {
						//shift y right, bit 0 goes to f (y in second 4 from left)
						printf("shift register %x 1 bit right, bit 0 goes to register f\n", ((opcode >> 8) & 0xf));
						reg[15] = reg[((opcode >> 8) & 0xf)] & 0x1;
						reg[((opcode >> 8) & 0xf)] >>= 1;
					} else if ((opcode & 0xf) == 7) {
						//subtract r from y, result in r, vf to 1 if borrows
						printf("subtract register %x from register %x, but put value in first, borrow in f\n", ((opcode >> 8) & 0xf), ((opcode >> 4) & 0xf));
						if (reg[((opcode >> 4) & 0xf)] > reg[((opcode >> 8) & 0xf)]) {
							reg[15] = 1;
							printf("borrow bit set\n");
						} else {
							reg[15] = 0;
						}
						reg[((opcode >> 8) & 0xf)] = reg[((opcode >> 8) & 0xf)] - reg[((opcode >> 4) & 0xf)];
					} else if ((opcode & 0xf) == 0xe) {
						//shift r left, bit 7 to f (r second 4 from left)
						printf("shift register %x 1 bit left, bit 7 into f\n", ((opcode >> 8) & 0xf));
						if (reg[((opcode >> 8) & 0xf)] >> 7) {
							reg[15] = 1;
						} else {
							reg[15] = 0;
						}
						reg[((opcode >> 8) & 0xf)] <<= 1;
					}
				} else if (opcode >> 12 == 9 && (opcode & 0xf) == 0) {
					//skip if r != y (ry middle 8)
					printf("skip if %x != %x\n", ((opcode >> 8) & 0xf), ((opcode >> 4) & 0xf));
					if (reg[((opcode >> 8) & 0xf)] != reg[((opcode >> 4) & 0xf)]) {
						printf("skipped\n");
						pc+=2;
					}
				} else if (opcode >> 12 == 0xa) {
					//load index reg with xxx (xxx last 12 bits)
					printf("load %d into register i\n", opcode & 0xfff);
					i = opcode & 0xfff;
				} else if (opcode >> 12 == 0xb) {
					//Jump to address xxx + v0 (xxx last 12 bits)
					printf("jump to address %x + register 0\n", opcode & 0xfff);
					pc = (opcode & 0xfff) + reg[0];
					pc-=2;
				} else if (opcode >> 12 == 0xc) {
					//r = random less than or equal to xx? (rxx last 12 bits)
					printf("random value placed in reg %x less than or equal to %d\n", ((opcode >> 8) & 0xf), ((opcode & 0xff)));
					reg[(opcode >> 8) & 0xf] = (rand() % 255) & (opcode & 0xff);
				} else if (opcode >> 12 == 0xd && (opcode & 0xf) != 0) {
					//draw sprite at screen location r, y, height s (rys last 12 bits),
					//Sprites stored at location of index reg, max 8 bits wide, wraps around screen, if, when drawn, clears a pixel, vf set to 1, otherwise it is 0, All drawing is xor drawing (toggles screen pixels)
					printf("draw %d byte sprite at pointed to by i at %d, %d\n", ((opcode >> 8) & 0xf), ((opcode >> 4) & 0xf), (opcode & 0xf));
					int x = reg[(opcode >> 8) & 0xf];
					int y = reg[(opcode >> 4) & 0xf];
					int n = (opcode & 0xf);
					int set = 0;
					reg[15] = 0;
					for (int ind = 0; ind < n; ind++) {
						if (x % 8 == 0) {
							/*if ((screen[(y + ind)][x / 8] & mem[i + ind] & 0xff) != 0) {
								set = 1;
								reg[15] = 1;
							}*/
							uint8_t old_val = screen[(y+ind)][x/8];
							screen[(y + ind)][(x/8)] ^= mem[i + ind];
							if (old_val != screen[y+ind][x/8]) {
								set=1;
								reg[15] = 1;
							}
						} else {
							int offset = x % 8;
							int x_round = (x >> 3);
							uint8_t old_val1 = screen[y+ind][x/8];
							uint8_t old_val2 = screen[y+ind][x/8+1];
							/*if ((screen[(y + ind)][(x_round - 1)] << offset) & ((mem[i+ind] >> offset)) & 0xff != 0) {
								set = 1;
								reg[15] = 1;
							}
							if (mem[i+ind] != 0) {
								//(((screen[(y + ind)][(x_round)] >> (8 - offset))) & (mem[i+ind] << (8 - offset - 1)) & 0xff != 0) {
								set = 1;
								reg[15] = 1;
							}*/
							screen[(y + ind)][(x_round)] ^= mem[i+ind] >> (offset );
							screen[(y + ind)][(x_round+1)] ^= mem[i+ind] << (8 - offset);
							if (old_val1 != screen[y+ind][x_round] || old_val2 != screen[y+ind][x_round+1]) {
								set=1;
								reg[15] = 1;
							}
						}
					}
				} else if (opcode >> 12 == 0xd) {
					//Same as above, but sprite is always 16x16, superchip only, not yet implemented
				} else if (opcode >> 12 == 0xe && (opcode & 0xff) == 0x9e) {
					//skip if key is pressed (k is second 4 bits from  left) (key is a key number)
					printf("skip if key %x is pressed\n", ((opcode >> 8) & 0xf));
					if (key[(opcode >>8) & 0xf]) {
						printf("skipped\n");
						pc+=2;
					}
				} else if (opcode >> 12 == 0xe && (opcode & 0xff) == 0xa1) {
					//skip if key is not pressed (k is second 4 from left)
					printf("skip if key %x is not pressed\n", ((opcode >> 8) & 0xf));
					if (!key[(opcode >>8) & 0xf]) {
						printf("skipped\n");
						pc+=2;
					}
				} else if (opcode >> 12 == 0xf && (opcode & 0xff) == 0x07) {
					//put delay timer into register r (r second 4 from left)
					printf("puts delay timer into register %x\n", ((opcode >> 8) & 0xf));
					reg[(opcode >> 8) & 0xf] = delay;
				} else if (opcode >> 12 == 0xf && (opcode & 0xff)== 0x0a) {
					//getwait for keypress, put key in register r (r second 4 from left)
					printf("getwait for keypress, then puts key in register %x\n", (opcode >> 8) & 0xf);
					int found = -1;
					for (int ind = 0; ind < 16; ind++) {
						if (key[ind] == 1 || key[ind] == 2) {
							found = ind;
						}
					}
					if (found != -1) {
						reg[(opcode >> 8) & 0xf] = found;
					} else {
						pc -= 2;
					}
				} else if (opcode >> 12 == 0xf &&(opcode & 0xff) == 0x15) {
					//set delay timer to vr
					printf("set delay timer to register %x\n", ((opcode >> 8) & 0xf));
					delay = reg[(opcode >> 8) & 0xf];
				} else if (opcode >> 12 == 0xf && (opcode & 0xff) == 0x18) {
					//Set sound timer to vr
					printf("set sound timer to register %x\n", ((opcode >> 8) & 0xf));
					sound = reg[(opcode >> 8) & 0xf];
				} else if (opcode >> 12 == 0xf && (opcode & 0xff) == 0x1e) {
					//Add register r to index register
					printf("add register %x to index register\n", ((opcode >> 8) & 0xf));
					i += reg[(opcode >> 8) & 0xf];
				} else if (opcode >> 12 == 0xf && (opcode & 0xff) == 0x29) {
					//point I to the sprite for the hexadecimal character in vr (Sprite is 5 bytes high)
					printf("point i to sprite for hexadecimal character in reg %x\n", ((opcode >> 8) & 0xf));
					i = ((reg[(opcode >> 8) & 0xf] % 16) * 5);
				} else if (opcode >> 12 == 0xf && (opcode & 0xff) == 0x30) {
					//Point I to the sprite for the hexadecimal character in vr (Sprite is 10 bytes high, Super only)
				} else if (opcode >> 12 == 0xf && (opcode & 0xff) == 0x33) {
					//store the bcd representation of register vr at location I,I+1,I+2, doesn't change I
					printf("store the bcd rep of register %x at location I, I+1, and I+2 in memory\n", ((opcode >> 8) & 0xf));
					uint8_t val = reg[(opcode >> 8) & 0xf];
					mem[i] = val / 100;
					mem[i+1] = (val / 10) % 10;
					mem[i+2] = val % 10;
				} else if (opcode >> 12 == 0xf && (opcode & 0xff) == 0x55) {
					//stores registers v0-vr at location I onwards
					//I in incremented I = I + r + 1
					printf("stores register 0 through register %x at location I onwards, I = I + r + 1\n", ((opcode >> 8) & 0xf));
					for (int ind = 0; ind <= ((opcode >> 8) & 0xf); ind++) {
						mem[i+ind] = reg[ind];
					}
					i += ((opcode >> 8) & 0xf) + 1;
				} else if (opcode >> 12 == 0xf && (opcode & 0xff) == 0x65) {
					printf("loads register 0 through register %x at location I onwards, I = I + r + 1\n", ((opcode >> 8) & 0xf));
					//load registers v0-vr from location I onwards
					// as above
					for (int ind = 0; ind <= ((opcode >> 8) & 0xf); ind++) {
						reg[ind] = mem[i+ind]; // (could be i - ind - 1)
					}
					i += ((opcode >> 8) & 0xf) + 1; //could be -=
				} else {
					printf("Unfortunately, the instruction this Chip attempted to process is not a valid Chip8 instruction\n");
				}
				break;
		}
		pc+=2;
		//i = !i;
		SDL_Event e;
		for (int ind = 0; ind < 16; ind++) {
			if (key[ind] == 1) {
				key[ind] = 2;
			} else if (key[ind] == 3) {
				key[ind] = 0;
			}
		}
		if (SDL_PollEvent(&e)) {
			if (e.type == SDL_QUIT) {
				break;
			} else if (e.type == SDL_KEYDOWN) {
				int ind = -1;
				switch(e.key.keysym.sym) {
					case SDLK_1:
						ind = 0;
						break;
					case SDLK_2:
						ind = 1;
						break;
					case SDLK_3:
						ind = 2;
						break;
					case SDLK_4:
						ind = 3;
						break;
					case SDLK_q:
						ind = 4;
						break;
					case SDLK_w:
						ind = 5;
						break;
					case SDLK_e:
						ind = 6;
						break;
					case SDLK_r:
						ind = 7;
						break;
					case SDLK_a:
						ind = 8;
						break;
					case SDLK_s:
						ind = 9;
						break;
					case SDLK_d:
						ind = 10;
						break;
					case SDLK_f:
						ind = 11;
						break;
					case SDLK_z:
						ind = 12;
						break;
					case SDLK_x:
						ind = 13;
						break;
					case SDLK_c:
						ind = 14;
						break;
					case SDLK_v:
						ind = 15;
						break;
					default:
						break;
				}
				if (ind != -1) {
					key[ind] = 1;
				}
			} else if (e.type == SDL_KEYUP) {
				int ind = -1;
				switch(e.key.keysym.sym) {
					case SDLK_1:
						ind = 0;
						break;
					case SDLK_2:
						ind = 1;
						break;
					case SDLK_3:
						ind = 2;
						break;
					case SDLK_4:
						ind = 3;
						break;
					case SDLK_q:
						ind = 4;
						break;
					case SDLK_w:
						ind = 5;
						break;
					case SDLK_e:
						ind = 6;
						break;
					case SDLK_r:
						ind = 7;
						break;
					case SDLK_a:
						ind = 8;
						break;
					case SDLK_s:
						ind = 9;
						break;
					case SDLK_d:
						ind = 10;
						break;
					case SDLK_f:
						ind = 11;
						break;
					case SDLK_z:
						ind = 12;
						break;
					case SDLK_x:
						ind = 13;
						break;
					case SDLK_c:
						ind = 14;
						break;
					case SDLK_v:
						ind = 15;
						break;
					default:
						break;
				}
				if (ind != -1) {
					key[ind] = 3;
				}
			}
		}
		if (reg[15] && (opcode >> 12) == 0xd) {
			for (int y = 0; y < 32; y++) {
				for (int x = 0; x < 8; x++) {
					for (int b = 0; b < 8; b++) {
						if ((screen[y][x] >> (8 - b -1)) & 0x1) {
							SDL_SetRenderDrawColor(ren, 255, 255, 255, SDL_ALPHA_OPAQUE);
						} else {
							SDL_SetRenderDrawColor(ren, 0, 0, 0, SDL_ALPHA_OPAQUE);
						}
						SDL_Rect rect;
						rect.x = (x * 8 + b) * 16;
						rect.y = (y) * 16;
						rect.w = 16;
						rect.h = 16;
						SDL_RenderFillRect(ren, &rect);
					}
				}
			}
			SDL_RenderPresent(ren);
			SDL_ShowWindow(win);
		}
		char a[5];
		//scanf("%c", a);
	}
	SDL_DestroyRenderer(ren);
	SDL_DestroyWindow(win);
	SDL_Quit();
	return 0;
}
