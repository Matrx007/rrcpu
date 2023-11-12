MVI $SS :stack_start
MVI $SP :stack_start
MVI $SF :stack_start

MVI $CB 'B'
MVI $CA 'X'
PUSH $CA

CALL :my_function
HALT

test_str1:
        .data b"Hello!\0"
test_str2:
        .data b"Heoo!\0"

my_function:
        MVI $A :test_str1
        MVI $B :test_str2
        MVI $CA 0u8

        PUSH $CA
        PUSH $A
        PUSH $B
        CALL :strcmp
        GLOC $CA 0u32
        JMPZ $CA :my_function_ok
        JMP :my_function_not_ok
my_function_ok:
        MVI $A "Strings match :)\n"
        PUSH $A
        CALL :print
        RET
my_function_not_ok:
        MVI $A "Strings don't match :(\n"
        PUSH $A
        CALL :print
        RET




print:
        GARG $MAR 0u32
print_loop:
        LDA
        JMPI $MDR :print_return 0u8
        OUT $MDR
        ADDI $MAR 1u32
        JMP :print_loop
print_return:
        RET



# strcmp(res: u8, str1: u32, str2: u32)
strcmp:
        GARG $MAR 0u32
        GARG $C 4u32
strcmp_loop:
        LDA
        MOV $CA $MDR
        MOV $D $MAR

        MOV $MAR $C
        LDA
        MOV $MAR $D

        JMPE $MDR $CA :strcmp_match
strcmp_return_fail:
        MVI $CA 1u8
        SARG $CA 5u32
        RET
strcmp_return_ok:
        MVI $CA 0u8
        SARG $CA 5u32
        RET
strcmp_match:
        JMPI $MDR :strcmp_return_ok '\0'
strcmp_advance_loop:
        ADDI $MAR 1u32
        ADDI $C 1u32
        JMP :strcmp_loop



HALT

MVI $MAR :test_path

test_loop:
MVI $C :test_file
MVI $A :after
JMP :fpathcmp

after:
        ADDI $B 0x41u32
        OUT $B
        OUTI '\n'
        JMPI $B :test_go_loop 0x42u32
        HALT
test_go_loop:
        ADDI $MAR 2u32
        JMP :test_loop


test_file:
        .data b"file4\0"
test_path:
        .data b"file1/file2/file3\0"





JMP :shell

MVI $A :halt
MVI $MAR :fs3
JMP :fs_list

halt: HALT

main:   MVI $MAR "Hello world!\n"
        JMP :print







shell:
        MVI $C :shell_got_command
        JMP :shell_prompt

shell_got_command:
        MVI $A :shell
        MVI $MAR :shell_blob
        JMP :print

shell_prompt:
        MVI $A :shell_prompt_path_printed
        JMP :shell_print_path
shell_prompt_path_printed:
        OUTI '$'
        OUTI ' '
        OUTI '>'
        OUTI ' '
        MVI $MAR :shell_blob
        MVI $CB '\n'
shell_loop:
        IN $MDR
        STA
        JMPI $MAR :shell_max_hit :shell_blob_end
        ADDI $MAR 1u32
        JMPI $MDR :shell_enter '\n'
        JMP :shell_loop
shell_max_hit:
        JMPI $MDR :shell_max_hit_text '\n'
        IN $MDR
        JMP :shell_max_hit
shell_max_hit_text:
        MOV $B $C
        MVI $MAR "Prompt too long\n"
        MVI $A :shell_max_hit_text_back
        JMP :print
shell_max_hit_text_back:
        MVI $MAR :shell_blob
        MVI $MDR 0u8
        STA
        JMPR $B
shell_enter:
        MVI $MDR '\0'
        STA
        JMPR $C



shell_print_path:
        MOV $D $A
        MVI $E :shell_path
        JMP :shell_print_path_item
shell_print_path_done:
        JMPR $D
shell_print_path_item:
        MOV $MAR $E
        MVI $A :shell_print_path_item_got_addr
        JMP :read32
shell_print_path_item_got_addr:
        JMPI $B :shell_print_path_done 0u32
        MOV $MAR $B
        ADDI $MAR 9u32
        MVI $A :shell_print_path_item_done
        OUTI '/'
        JMP :print
shell_print_path_item_done:
        ADDI $E 4u32
        JMP :shell_print_path_item



shell_blob:
        .empty 256
shell_blob_end:
shell_blob_params_p:
        .data 0u32



shell_path:
        .data :fs_root
        .empty 252
shell_path_top:
        .data :shell_path











print_old:  LDA
        JMPI $MDR :return 0u8
        OUT $MDR
        ADDI $MAR 1u32
        JMP :print_old
return: JMPR $A


# MAR - addr, B - output value, A - return to
read32:
        MVI $B 0u32
        
        LDA
        ADD $B $MDR
        ROTR $B 8u8
        ADDI $MAR 1u32
        
        LDA
        ADD $B $MDR
        ROTR $B 8u8
        ADDI $MAR 1u32
        
        LDA
        ADD $B $MDR
        ROTR $B 8u8
        ADDI $MAR 1u32
        
        LDA
        ADD $B $MDR
        ROTR $B 8u8

        JMPR $A


# MAR - str 1, B - result, 0 if match, C - str 2, A - return addr, (D also used)
old_strcmp:
old_strcmp_loop:
        LDA
        MOV $CA $MDR
        MOV $D $MAR

        MOV $MAR $C
        LDA
        MOV $MAR $D

        JMPE $MDR $CA :old_strcmp_match
old_strcmp_return_fail:
        MVI $B 1u32
        JMPR $A
old_strcmp_return_ok:
        MVI $B 0u32
        JMPR $A
old_strcmp_match:
        JMPI $MDR :old_strcmp_return_ok '\0'
old_strcmp_advance_loop:
        ADDI $MAR 1u32
        ADDI $C 1u32
        JMP :old_strcmp_loop
        



# MAR - path 1, B - result, 0 if match, C - file name, A - return addr, (D also used)
fpathcmp:
fpathcmp_loop:
        LDA
        MOV $CA $MDR
        MOV $D $MAR

        MOV $MAR $C
        LDA
        MOV $MAR $D

        JMPI $CA :fpathcmp_return_ok '/'
        JMPE $MDR $CA :fpathcmp_match
fpathcmp_return_fail:
        MVI $B 1u32
        JMPR $A
fpathcmp_return_ok:
        MVI $B 0u32
        JMPR $A
fpathcmp_match:
        JMPI $MDR :fpathcmp_return_ok '\0'
fpathcmp_advance_loop:
        ADDI $MAR 1u32
        ADDI $C 1u32
        JMP :fpathcmp_loop






# MAR - file addr, A - return addr
fs_list:
        MOV $D $A
        LDA
        JMPI $MDR :fs_list_dir 'd'
        JMPI $MDR :fs_list_file 'f'
        MVI $MAR "Unknown file type"
        MOV $A $D
        JMP :print
fs_list_done:
        JMPR $D
fs_list_dir:
        ADDI $MAR 1u32
fs_list_dir_item:
        MOV $B $MAR
        ADDI $MAR 8u32
        MVI $A :fs_list_dir_item_printed
        JMP :print
fs_list_dir_item_printed:
        OUTI '\n'
        MOV $MAR $B
        MVI $A :fs_list_dir_item_go_next
        JMP :read32
fs_list_dir_item_go_next:
        JMPI $B :fs_list_done 0u32
        MOV $MAR $B
        JMP :fs_list_dir_item
fs_list_file:
        MOV $A $D
        MVI $MAR "Expected directory, can't list file"
        JMP :print





# MAR - path str, B - result: addr, A - return addr
fs_deref:
        






.empty 256

fs_root:

fs0:            .data 'd'
fs0_e0:         .data :fs0_e1
                .data :fs1
                .data b"text1.txt\0"
fs0_e1:         .data :fs0_e2
                .data :fs2
                .data b"text2.txt\0"
fs0_e2:         .data 0u32
                .data :fs3
                .data b"bin\0"

fs1:            .data 'f'
                .data 30u32
                .data b"Hellooooooo, this is a file!!\0"

fs2:            .data 'f'
                .data 19u32
                .data b"This da othe filee\0"

fs3:            .data 'd'
fs3_e0:         .data :fs3_e1
                .data :fs4
                .data b"arch\0"
fs3_e1:         .data 0u32
                .data :fs5
                .data b"ping\0"

fs4:            .data 'f'
                .data 24u32
                .data b"<pretend to be binary>\0"

fs5:            .data 'f'
                .data 24u32
                .data b"<pretend to be binary>\0"


.empty 256



.empty 2048
stack_start: