#include "font.h"

// TODO: Move this from a function to static data
void store_font(uint32_t *buffer) {
  buffer[0]   = 0b00111100;
  buffer[1]   = 0b01000010;
  buffer[2]   = 0b01000010;
  buffer[3]   = 0b01000010;
  buffer[4]   = 0b01111110;
  buffer[5]   = 0b01000010;
  buffer[6]   = 0b01000010;
  buffer[7]   = 0b01000010;
 
  buffer[8]   = 0b01111100;
  buffer[9]   = 0b01000010;
  buffer[10]  = 0b01000010;
  buffer[11]  = 0b01111100;
  buffer[12]  = 0b01111100;
  buffer[13]  = 0b01000010;
  buffer[14]  = 0b01000010;
  buffer[15]  = 0b01111100;
 
  buffer[16]  = 0b00011110;
  buffer[17]  = 0b00100000;
  buffer[18]  = 0b01000000;
  buffer[19]  = 0b01000000;
  buffer[20]  = 0b01000000;
  buffer[21]  = 0b01000000;
  buffer[22]  = 0b00100000;
  buffer[23]  = 0b00011110;
 
  buffer[24]  = 0b01111000;
  buffer[25]  = 0b01000100;
  buffer[26]  = 0b01000010;
  buffer[27]  = 0b01000010;
  buffer[28]  = 0b01000010;
  buffer[29]  = 0b01000010;
  buffer[30]  = 0b01000100;
  buffer[31]  = 0b01111000;
 
  buffer[32]  = 0b01111110;
  buffer[33]  = 0b01000000;
  buffer[34]  = 0b01000000;
  buffer[35]  = 0b01000000;
  buffer[36]  = 0b01111000;
  buffer[37]  = 0b01000000;
  buffer[38]  = 0b01000000;
  buffer[39]  = 0b01111110;
 
  buffer[40]  = 0b01111110;
  buffer[41]  = 0b01000000;
  buffer[42]  = 0b01000000;
  buffer[43]  = 0b01000000;
  buffer[44]  = 0b01111000;
  buffer[45]  = 0b01000000;
  buffer[46]  = 0b01000000;
  buffer[47]  = 0b01000000;
 
  buffer[48]  = 0b00111100;
  buffer[49]  = 0b01000010;
  buffer[50]  = 0b01000000;
  buffer[51]  = 0b01000000;
  buffer[52]  = 0b01001111;
  buffer[53]  = 0b01000010;
  buffer[54]  = 0b01000010;
  buffer[55]  = 0b00111100;
 
  buffer[56]  = 0b01000010;
  buffer[57]  = 0b01000010;
  buffer[58]  = 0b01000010;
  buffer[59]  = 0b01000010;
  buffer[60]  = 0b01111110;
  buffer[61]  = 0b01000010;
  buffer[62]  = 0b01000010;
  buffer[63]  = 0b01000010;
 
  buffer[64]  = 0b01111110;
  buffer[65]  = 0b00001000;
  buffer[66]  = 0b00001000;
  buffer[67]  = 0b00001000;
  buffer[68]  = 0b00001000;
  buffer[69]  = 0b00001000;
  buffer[70]  = 0b00001000;
  buffer[71]  = 0b01111110;
 
  buffer[72]  = 0b01111110;
  buffer[73]  = 0b00001000;
  buffer[74]  = 0b00001000;
  buffer[75]  = 0b00001000;
  buffer[76]  = 0b00001000;
  buffer[77]  = 0b01001000;
  buffer[78]  = 0b01001000;
  buffer[79]  = 0b00110000;
 
  buffer[80]  = 0b01000100;
  buffer[81]  = 0b01001000;
  buffer[82]  = 0b01010000;
  buffer[83]  = 0b01100000;
  buffer[84]  = 0b01100000;
  buffer[85]  = 0b01010000;
  buffer[86]  = 0b01001000;
  buffer[87]  = 0b01000100;
 
  buffer[88]  = 0b01110000;
  buffer[89]  = 0b00100000;
  buffer[90]  = 0b00100000;
  buffer[91]  = 0b00100000;
  buffer[92]  = 0b00100000;
  buffer[93]  = 0b00100000;
  buffer[94]  = 0b00100000;
  buffer[95]  = 0b01111110;
 
  buffer[96]  = 0b01000010;
  buffer[97]  = 0b01100110;
  buffer[98]  = 0b01011010;
  buffer[99]  = 0b01000010;
  buffer[100] = 0b01000010;
  buffer[101] = 0b01000010;
  buffer[102] = 0b01000010;
  buffer[103] = 0b01000010;

  buffer[104] = 0b01000010;
  buffer[105] = 0b01000010;
  buffer[106] = 0b01100010;
  buffer[107] = 0b01010010;
  buffer[108] = 0b01001010;
  buffer[109] = 0b01000110;
  buffer[110] = 0b01000010;
  buffer[111] = 0b01000010;

  buffer[112] = 0b00111100;
  buffer[113] = 0b01000010;
  buffer[114] = 0b01000010;
  buffer[115] = 0b01000010;
  buffer[116] = 0b01000010;
  buffer[117] = 0b01000010;
  buffer[118] = 0b01000010;
  buffer[119] = 0b00111100;

  buffer[120] = 0b01111100;
  buffer[121] = 0b01000010;
  buffer[122] = 0b01000010;
  buffer[123] = 0b01000010;
  buffer[124] = 0b01111100;
  buffer[125] = 0b01000000;
  buffer[126] = 0b01000000;
  buffer[127] = 0b01000000;

  buffer[128] = 0b00111100;
  buffer[129] = 0b01000010;
  buffer[130] = 0b01000010;
  buffer[131] = 0b01000010;
  buffer[132] = 0b01000010;
  buffer[133] = 0b01001010;
  buffer[134] = 0b01000110;
  buffer[135] = 0b00111100;

  buffer[136] = 0b01111100;
  buffer[137] = 0b01000010;
  buffer[138] = 0b01000010;
  buffer[139] = 0b01111100;
  buffer[140] = 0b01010000;
  buffer[141] = 0b01001000;
  buffer[142] = 0b01000100;
  buffer[143] = 0b01000010;

  buffer[144] = 0b00111100;
  buffer[145] = 0b01000000;
  buffer[146] = 0b01000000;
  buffer[147] = 0b01000000;
  buffer[148] = 0b00111100;
  buffer[149] = 0b00000010;
  buffer[150] = 0b00000010;
  buffer[151] = 0b00111100;

  buffer[152] = 0b01111110;
  buffer[153] = 0b00001000;
  buffer[154] = 0b00001000;
  buffer[155] = 0b00001000;
  buffer[156] = 0b00001000;
  buffer[157] = 0b00001000;
  buffer[158] = 0b00001000;
  buffer[159] = 0b00001000;

  buffer[160] = 0b01000010;
  buffer[161] = 0b01000010;
  buffer[162] = 0b01000010;
  buffer[163] = 0b01000010;
  buffer[164] = 0b01000010;
  buffer[165] = 0b01000010;
  buffer[166] = 0b01000010;
  buffer[167] = 0b00111100;

  buffer[168] = 0b01000010;
  buffer[169] = 0b01000010;
  buffer[170] = 0b01000010;
  buffer[171] = 0b01000010;
  buffer[172] = 0b01000010;
  buffer[173] = 0b01000010;
  buffer[174] = 0b00100100;
  buffer[175] = 0b00011000;

  buffer[176] = 0b01000010;
  buffer[177] = 0b01000010;
  buffer[178] = 0b01000010;
  buffer[179] = 0b01000010;
  buffer[180] = 0b01000010;
  buffer[181] = 0b01011010;
  buffer[182] = 0b01100110;
  buffer[183] = 0b01000010;

  buffer[184] = 0b01000010;
  buffer[185] = 0b01000010;
  buffer[186] = 0b00100100;
  buffer[187] = 0b00011000;
  buffer[188] = 0b00011000;
  buffer[189] = 0b00100100;
  buffer[190] = 0b01000010;
  buffer[191] = 0b01000010;

  buffer[192] = 0b01000010;
  buffer[193] = 0b01000010;
  buffer[194] = 0b00100100;
  buffer[195] = 0b00011000;
  buffer[196] = 0b00010000;
  buffer[197] = 0b00010000;
  buffer[198] = 0b00010000;
  buffer[199] = 0b00010000;

  buffer[200] = 0b01111110;
  buffer[201] = 0b00000010;
  buffer[202] = 0b00000100;
  buffer[203] = 0b00001000;
  buffer[204] = 0b00010000;
  buffer[205] = 0b00100000;
  buffer[206] = 0b01000000;
  buffer[207] = 0b01111110;

  buffer[208] = 0b00111100;
  buffer[209] = 0b01000010;
  buffer[210] = 0b01000110;
  buffer[211] = 0b01001010;
  buffer[212] = 0b01010010;
  buffer[213] = 0b01100010;
  buffer[214] = 0b01000010;
  buffer[215] = 0b00111100;

  buffer[216] = 0b00001000;
  buffer[217] = 0b00011000;
  buffer[218] = 0b00101000;
  buffer[219] = 0b00001000;
  buffer[220] = 0b00001000;
  buffer[221] = 0b00001000;
  buffer[222] = 0b00001000;
  buffer[223] = 0b01111110;

  buffer[224] = 0b00111100;
  buffer[225] = 0b01000010;
  buffer[226] = 0b00000010;
  buffer[227] = 0b00000100;
  buffer[228] = 0b00001000;
  buffer[229] = 0b00010000;
  buffer[230] = 0b00100000;
  buffer[231] = 0b01111110;

  buffer[232] = 0b01111100;
  buffer[233] = 0b00000010;
  buffer[234] = 0b00000010;
  buffer[235] = 0b00000010;
  buffer[236] = 0b01111100;
  buffer[237] = 0b00000010;
  buffer[238] = 0b00000010;
  buffer[239] = 0b01111100;

  buffer[240] = 0b00000110;
  buffer[241] = 0b00001010;
  buffer[242] = 0b00010010;
  buffer[243] = 0b00100010;
  buffer[244] = 0b01111110;
  buffer[245] = 0b00000010;
  buffer[246] = 0b00000010;
  buffer[247] = 0b00000010;

  buffer[248] = 0b01111110;
  buffer[249] = 0b01000000;
  buffer[250] = 0b01000000;
  buffer[251] = 0b00111100;
  buffer[252] = 0b00000010;
  buffer[253] = 0b00000010;
  buffer[254] = 0b00000010;
  buffer[255] = 0b01111100;

  buffer[256] = 0b00111110;
  buffer[257] = 0b01000000;
  buffer[258] = 0b01000000;
  buffer[259] = 0b01000000;
  buffer[260] = 0b01111100;
  buffer[261] = 0b01000010;
  buffer[262] = 0b01000010;
  buffer[263] = 0b00111100;

  buffer[264] = 0b01111110;
  buffer[265] = 0b01000010;
  buffer[266] = 0b00000010;
  buffer[267] = 0b00000100;
  buffer[268] = 0b00111110;
  buffer[269] = 0b00010000;
  buffer[270] = 0b00010000;
  buffer[271] = 0b00010000;

  buffer[272] = 0b00111100;
  buffer[273] = 0b01000010;
  buffer[274] = 0b01000010;
  buffer[275] = 0b01000010;
  buffer[276] = 0b00111100;
  buffer[277] = 0b01000010;
  buffer[278] = 0b01000010;
  buffer[279] = 0b00111100;

  buffer[280] = 0b00111100;
  buffer[281] = 0b01000010;
  buffer[282] = 0b01000010;
  buffer[283] = 0b00111110;
  buffer[284] = 0b00000010;
  buffer[285] = 0b00000010;
  buffer[286] = 0b00000010;
  buffer[287] = 0b00000010;

  buffer[288] = 0b00000000;
  buffer[289] = 0b00000000;
  buffer[290] = 0b00000000;
  buffer[291] = 0b00000000;
  buffer[292] = 0b00000000;
  buffer[293] = 0b00000000;
  buffer[294] = 0b00000000;
  buffer[295] = 0b00000000;

  buffer[296] = 0b00011000;
  buffer[297] = 0b00011000;
  buffer[298] = 0b00011000;
  buffer[299] = 0b00011000;
  buffer[300] = 0b00011000;
  buffer[301] = 0b00000000;
  buffer[302] = 0b00011000;
  buffer[303] = 0b00011000;

  buffer[296] = 0b01100110;
  buffer[297] = 0b01100110;
  buffer[298] = 0b01100110;
  buffer[299] = 0b00000000;
  buffer[300] = 0b00000000;
  buffer[301] = 0b00000000;
  buffer[302] = 0b00000000;
  buffer[303] = 0b00000000;

  buffer[296] = 0b00100100;
  buffer[297] = 0b00100100;
  buffer[298] = 0b01111110;
  buffer[299] = 0b00100100;
  buffer[300] = 0b00100100;
  buffer[301] = 0b01111110;
  buffer[302] = 0b00100100;
  buffer[303] = 0b00100100;

  buffer[296] = 0b00011000;
  buffer[297] = 0b00111110;
  buffer[298] = 0b01011000;
  buffer[299] = 0b01011000;
  buffer[300] = 0b00111100;
  buffer[301] = 0b00011010;
  buffer[302] = 0b01111100;
  buffer[303] = 0b00011000;

  buffer[296] = 0b01100000;
  buffer[297] = 0b01100010;
  buffer[298] = 0b00000100;
  buffer[299] = 0b00001000;
  buffer[300] = 0b00010000;
  buffer[301] = 0b00100000;
  buffer[302] = 0b01000110;
  buffer[303] = 0b00000110;

  buffer[296] = 0b00111100;
  buffer[297] = 0b01000010;
  buffer[298] = 0b01000100;
  buffer[299] = 0b00101000;
  buffer[300] = 0b00010000;
  buffer[301] = 0b00101010;
  buffer[302] = 0b01000100;
  buffer[303] = 0b00111010;

  buffer[296] = 0b00011000;
  buffer[297] = 0b00011000;
  buffer[298] = 0b00011000;
  buffer[299] = 0b00000000;
  buffer[300] = 0b00000000;
  buffer[301] = 0b00000000;
  buffer[302] = 0b00000000;
  buffer[303] = 0b00000000;

  buffer[296] = 0b00001100;
  buffer[297] = 0b00011000;
  buffer[298] = 0b00110000;
  buffer[299] = 0b01100000;
  buffer[300] = 0b01100000;
  buffer[301] = 0b00110000;
  buffer[302] = 0b00011000;
  buffer[303] = 0b00001100;

  buffer[296] = 0b00110000;
  buffer[297] = 0b00011000;
  buffer[298] = 0b00001100;
  buffer[299] = 0b00000110;
  buffer[300] = 0b00000110;
  buffer[301] = 0b00001100;
  buffer[302] = 0b00011000;
  buffer[303] = 0b00110000;

  buffer[296] = 0b01000010;
  buffer[297] = 0b01000010;
  buffer[298] = 0b00100100;
  buffer[299] = 0b00011000;
  buffer[300] = 0b01111110;
  buffer[301] = 0b00100100;
  buffer[302] = 0b01000010;
  buffer[303] = 0b01000010;

  buffer[296] = 0b00011000;
  buffer[297] = 0b00011000;
  buffer[298] = 0b00011000;
  buffer[299] = 0b01111110;
  buffer[300] = 0b01111110;
  buffer[301] = 0b00011000;
  buffer[302] = 0b00011000;
  buffer[303] = 0b00011000;

  buffer[296] = 0b00000000;
  buffer[297] = 0b00000000;
  buffer[298] = 0b00000000;
  buffer[299] = 0b00000000;
  buffer[300] = 0b00000000;
  buffer[301] = 0b00011000;
  buffer[302] = 0b00011000;
  buffer[303] = 0b00110000;

  buffer[296] = 0b00000000;
  buffer[297] = 0b00000000;
  buffer[298] = 0b00000000;
  buffer[299] = 0b01111110;
  buffer[300] = 0b01111110;
  buffer[301] = 0b00000000;
  buffer[302] = 0b00000000;
  buffer[303] = 0b00000000;

  buffer[296] = 0b00000000;
  buffer[297] = 0b00000000;
  buffer[298] = 0b00000000;
  buffer[299] = 0b00000000;
  buffer[300] = 0b00000000;
  buffer[301] = 0b00111000;
  buffer[302] = 0b00111000;
  buffer[303] = 0b00111000;

  buffer[296] = 0b00000000;
  buffer[297] = 0b00000010;
  buffer[298] = 0b00000100;
  buffer[299] = 0b00001000;
  buffer[300] = 0b00010000;
  buffer[301] = 0b00100000;
  buffer[302] = 0b01000000;
  buffer[303] = 0b00000000;

}