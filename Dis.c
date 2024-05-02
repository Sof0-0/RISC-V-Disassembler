#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>

int main(int argc, char* argv[0]){
    //TODO: figure out offset for jal and branching

    //handle command line inputs 
    //argv[4] to be continued in sim... [-Tm:n].
    char* input = argv[1];
    char* output = argv[2]; 
    char* oper = argv[3]; //dis or sim (TBD)

    //Getting the original input
    FILE *p;
    int bits[32], bit_count, bit, opcode[512], func3[512], rd[512], rs[512], imm[512], rs2[512];
    int flag_zeros, zero_rows, address, data_rows;
    int in_count, special[32], offset[512], off_sw[32], of, jump[512];
    p = fopen(input, "r");

    if (p == NULL){ 
        printf("No file found.\n");
        printf("Please type the command as follows: RISC-Vsim inputfilename outputfilename operation.\n");
        return 1;
        }

    FILE *out = fopen(output, "w");
    if (out == NULL) {
        printf("Failed to open output file.\n");
        printf("Please type the command as follows: RISC-Vsim inputfilename outputfilename operation.\n");
        return 1;
    }

    if (oper == NULL || strcmp(oper, "dis") != 0 && strcmp(oper, "sim") != 0){
        printf("No valid operation provided(sim or dis).\n");
        printf("Please type the command as follows: RISC-Vsim inputfilename outputfilename operation.\n");
        return 1;
    }
    else if(strcmp(oper, "sim") == 0){
        printf("To be implemented soon!\n");
        return 1;
    }
   
    bit_count = 0;
    zero_rows = 0;
    data_rows = 0; //for further file formatting
    address = 496; //instructions begin at address 496
    in_count = 0;

    // initialize opcode and func3 arrays to zero
    for (int i = 0; i < 512; i++) {
        opcode[i] = 0;
        func3[i] = 0;
    }

    //file formatting to produce output.txt with columns 6,6,5,3,5,7 bits
    while ((bit = fgetc(p)) != EOF) {
        if (bit == '0' || bit == '1'){
            bits[bit_count++] = bit == '1' ? 1: 0; // store bit as integer (0 or 1)
            if (bit_count == 32) {
                // check if all bits are 0s
                int all_zeros = 1;
                for (int i = 0; i < 32; i++) {
                    if (bits[i] != 0) {
                        all_zeros = 0;
                        break;
                    }
                }
                if (all_zeros) { zero_rows++; } // increment count of consecutive all-zero rows
                else {
                    // write 6+6+5+3+5+7=32 bits to output file in separate columns

                    //special cursor handles some flags for instructions (like SUB vs ADD where the difference is in 1 bit)
                    int current_special = 0;
                    for (int i = 0; i < 6; i++) {
                        fprintf(out, "%d", bits[i]);
                        current_special = (current_special << 1) | bits[i];
                    }
                    fprintf(out, " ");
                    for (int i = 6; i < 12; i++) {
                        fprintf(out, "%d", bits[i]);
                    }
                    fprintf(out, " ");

                    //collect the immediate for I-instructions
                    //bits 31-20
                    int current_imm = 0;
                    for (int i = 0; i < 12; i++){current_imm = (current_imm << 1) | bits[i];}

                    //we want to check if the value collected above is negative
                    //OR with 0xFFFFF000 sign-extends the negative values by setting up 20 MSBs to 1s
                    //^ since we want to preserve 12 LSBs from the immediate
                    if(bits[0] == 1) { current_imm = current_imm | 0xFFFFF000;}

                    //collect rs2 for branch and boolean operation instructions
                    //bits 24-20
                    int current_rs2 = 0;
                    for (int i = 7; i < 12; i++){
                        current_rs2 = (current_rs2 << 1) | bits[i];
                    }

                    int current_rs = 0;
                    for (int i = 12; i < 17; i++) {
                        fprintf(out, "%d", bits[i]);
                        current_rs = (current_rs << 1) | bits[i]; //to get rs
                    }
                    fprintf(out, " ");

                    int current_func3 = 0;
                    for (int i = 17; i < 20; i++) {
                        fprintf(out, "%d", bits[i]);
                        current_func3 = (current_func3 << 1) | bits[i]; //to get func3
                    }
                    fprintf(out, " ");

                    int current_rd = 0; //11-7 bits for rd
                    for (int i = 20; i < 25; i++) {
                        fprintf(out, "%d", bits[i]);
                        current_rd = (current_rd << 1) | bits[i]; //to get the rd
                    }
                    fprintf(out, " ");

                    int current_op = 0; //6-0 bits for opcode
                    for (int i = 25; i < 32; i++) {
                        fprintf(out, "%d", bits[i]);
                        current_op = (current_op << 1) | bits[i]; //to get the opcode
                    }
                    
                    //collect the offset for branching instructions
                    //offset for branch instruction : [[12 | 10:5] | [4:1 | 11]]
                    int current_offset = 0;
                    for (int i = 0; i <= 19; i++){current_offset = (current_offset << 1) | bits[0];} //copy the sign bit to the first 20 bits
                    current_offset = (current_offset << 1) | bits[24]; //push bits[24]
                    for (int i = 1; i <= 6; i++){current_offset = (current_offset << 1) | bits[i]; } //push [10:5] as they come first
                    for (int i = 20; i <= 23; i++){current_offset = (current_offset << 1) | bits[i]; } //push [4:1] as they come after
                    current_offset = (current_offset << 1) | 0; //assign the 32nd bit to 0 (for all B-instructions)

                    //collect the offset for jump instructions
                    //offset for jump instruction : [[20 | 10:1] | [11 | 19:12]]
                    int current_jump = 0;
                    for (int i = 0; i <= 11; i++){current_jump = (current_jump << 1) | bits[0];} //copy the sign bit to the first 12 bits
                    for (int i = 12; i <= 19; i++){current_jump = (current_jump << 1) | bits[i]; } //push [19:12] as they come first
                    current_jump = (current_jump << 1) | bits[11]; //push bits[11]
                    for (int i = 1; i <= 10; i++){current_jump = (current_jump << 1) | bits[i];} //push [10:1] as they come after 11th bit
                    current_jump = (current_jump << 1) | 0; //assign the 32nd bit to 0 (for all J-instructions)
            
                    fprintf(out, "\t%d", address); //adding address row
                    address += 4; //incremeby by 4 for the next row
                    data_rows += 1; //the data was processed
    
                    // store current opcode and func3 in array
                    opcode[in_count] = current_op; //6-0 bits
                    rd[in_count] = current_rd; //11-7 bits
                    func3[in_count] = current_func3; //14-12 bits
                    rs[in_count] = current_rs; //19-15 bits (rs1 for branch instructions)
                    rs2[in_count] = current_rs2; //24-20 bits (rs2 for branch and boolean instr)
                    imm[in_count] = current_imm; //31-20 bits
                    offset[in_count] = current_offset; //[[12|10:5]|[4:1|11]] bits
                    special[in_count] = current_special; //31-27 bits
                    jump[in_count] = current_jump; //[20|[10:1]|11|[19:12]] bits
                    fprintf(out, "\t");

                    //opcode decoding
                    switch(opcode[in_count]){
                        case 19:
                            //check for 14-12 (if 010 - SLTI)
                            if(func3[in_count] == 2){
                                fprintf(out, "SLTI\t");
                                fprintf(out, "x%d, x%d, %d\n", rd[in_count], rs[in_count], imm[in_count]);
                                break;
                            }
                            else{
                                //check for NOP operation when imm = 0:
                                if (imm[in_count] == 0) {
                                    fprintf(out, "NOP\t\t");
                                    fprintf(out, "//ADDI x%d, x%d, %d\n", rd[in_count], rs[in_count],imm[in_count]);
                                    break;
                                }
                                else{
                                    fprintf(out, "ADDI\t"); 
                                    fprintf(out, "x%d, x%d, %d\n", rd[in_count], rs[in_count],imm[in_count]);
                                    break;
                                }
                            }
                        case 35:
                            fprintf(out, "SW\t");
                            fprintf(out, "x%d, %d(x%d)\n", rs2[in_count], offset[in_count], rs[in_count]);
                            break;
                        case 3:
                            fprintf(out, "LW\t");
                            fprintf(out, "x%d, %d(x%d)\n", rd[in_count], imm[in_count], rs[in_count]);
                            break;
                        case 99:
                            //check for 14-12 (if 001 - BNE, if 100 - BLT, if 101 - BGE)
                            if(func3[in_count] == 1){
                                fprintf(out, "BNE\t");
                                fprintf(out, "x%d, x%d, %d\n", rs[in_count], rs2[in_count], offset[in_count]);
                                break;
                            }
                            else if(func3[in_count] == 4){
                                fprintf(out, "BLT\t");
                                fprintf(out, "x%d, x%d, %d\n", rs[in_count], rs2[in_count], offset[in_count]);
                                break;
                            }
                            else if(func3[in_count] == 5){
                                fprintf(out, "BGE\t");
                                fprintf(out, "x%d, x%d, %d\n", rs[in_count], rs2[in_count], offset[in_count]);
                                break;
                            }
                            else{
                                fprintf(out, "BEQ\t"); 
                                fprintf(out, "x%d, x%d, %d\n", rs[in_count], rs2[in_count], offset[in_count]);
                                break;
                            }
                        case 111:
                            //we extend the sign for the immediate (done during the calculation of imm[in_count])
                            //in the definition LSB for JAL must be set to 0, so we must adjust the offset
                            of = jump[in_count]; 
                            fprintf(out, "J\t"); 
                            fprintf(out, "#%d\t//JAL x%d, %d\n", (address - 4 + of), rd[in_count], of);
                            break; 


                        case 51:
                            //check for 14-12(if 010 - SLT, if 001 - SLL, if 101 - SRL, if 110 - OR, if 111 - AND, if 100 - XOR)
                            //check for 31-27 (if 010000 - SUB, else - ADD )
                            if(special[in_count] != 0){
                                fprintf(out, "SUB\t");
                                fprintf(out, "x%d, x%d, x%d\n", rd[in_count], rs[in_count], rs2[in_count]);
                                break;
                            }
                            else{
                                if(func3[in_count] == 1){
                                    fprintf(out, "SLL\t");
                                    fprintf(out, "x%d, x%d, x%d\n", rd[in_count], rs[in_count], rs2[in_count]);
                                    break;
                                }
                                else if(func3[in_count] == 2){
                                    fprintf(out, "SLT\t");
                                    fprintf(out, "x%d, x%d, x%d\n", rd[in_count], rs[in_count], rs2[in_count]);
                                    break;
                                }
                                else if(func3[in_count] == 4){
                                    fprintf(out, "XOR\t");
                                    fprintf(out, "x%d, x%d, x%d\n", rd[in_count], rs[in_count], rs2[in_count]);
                                    break;
                                }
                                else if(func3[in_count] == 5){
                                    fprintf(out, "SRL\t");
                                    fprintf(out, "x%d, x%d, x%d\n", rd[in_count], rs[in_count], rs2[in_count]);
                                    break;
                                }
                                else if(func3[in_count] == 6){
                                    fprintf(out, "OR\t");
                                    fprintf(out, "x%d, x%d, x%d\n", rd[in_count], rs[in_count], rs2[in_count]);
                                    break;
                                }
                                else if(func3[in_count] == 7){
                                    fprintf(out, "AND\t");
                                    fprintf(out, "x%d, x%d, x%d\n", rd[in_count], rs[in_count], rs2[in_count]);
                                    break;
                                }
                                else{
                                    fprintf(out, "ADD\t"); 
                                    fprintf(out, "x%d, x%d, x%d\n", rd[in_count], rs[in_count], rs2[in_count]);
                                    break;
                                }
                            }
                        case 103:
                            fprintf(out, "RET\t\t"); //JALR x0, x1, 0
                            fprintf(out, "//JALR x%d, x%d, %d\n", rd[in_count], rs[in_count], imm[in_count]);
                            break;
                        default:
                            fprintf(out, "TBD\n");
                            break;
                    }
                    in_count++;
                }
                bit_count = 0; // reset bit count for next line
            }
        }
    }

    // write remaining all-zero rows (if any)
    //treating zero's rows separately due to the absence of spaces 

    for (int i = 0; i < zero_rows; i++) {
        for (int j = 0; j < 32; j++) {
            fprintf(out, "0");
        }
        fprintf(out, "\t%d", address);
        address += 4;
        fprintf(out, "\t0\n"); //for opcode
    }
    fclose(p);
    fclose(out);

    printf("Output written to file 'output.txt'.\n");
    return 0;
}
