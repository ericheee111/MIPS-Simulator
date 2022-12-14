#include "catch.hpp"

#include "lexer.hpp"
#include "parser.hpp"
//#include <sstream>


// put your parser tests here
TEST_CASE("debug") {
	std::string input = R"(
        .data
        .space 8
var1:   .word 1
var2:   .word -2

        .text
main:
     	la $t0, var1

	lw $t1, 0
	lw $t2, $t0
	lw $t3, 4($t0)
	lw $t4, 4(var1)
	lw $t5, var2
	
                )";
	std::istringstream iss(input);

	TokenList tl = tokenize(iss);
	Parse syntax;
	REQUIRE(syntax.parse(tl) == true);
	VirtualMachine VM = syntax.getVM();
	std::vector<Instruction> instr = VM.getInstrVector();
	REQUIRE(instr.size() == 6);
}

TEST_CASE("test .data -- constant", "[parser]") {

	{
		std::string input = R"(
	# a test for basic data movement
	.data
avar:	.word 	0, 1, 2, 3
bvar:	.half	1, 2, 3, 4
cvar:	.byte	3, 4, 5, 6
dvar:	.space	5, 6, 7, 8

	.text
main:	
	li $t0, 45

	lw $t1, avar
	lh $t2, bvar
	lb $t2, cvar
	sw $t1, avar
	sh $t2, bvar
	sb $t2, dvar

	move $t0, $0
                )";
		std::istringstream iss(input);

		TokenList tl = tokenize(iss);
		Parse syntax;
		REQUIRE(syntax.parse(tl) == true);
	}


	{
		std::string input = R"(
            .data)";
		std::istringstream iss(input);

		TokenList tl = tokenize(iss);
		Parse syntax;
		REQUIRE(syntax.parse(tl) == true);
	}

	{
		std::string input = R"(
            .data
            NAME1 = 1
            NAME2 = 2
            NAME3 = -3)";
		std::istringstream iss(input);

		TokenList tl = tokenize(iss);
		Parse syntax;
		REQUIRE(syntax.parse(tl) == true);
	}

	{
		std::string input = R"(
            .data
            NAME1
            )";
		std::istringstream iss(input);

		TokenList tl = tokenize(iss);
		Parse syntax;
		REQUIRE(syntax.parse(tl) == false);
	}

	{
		std::string input = R"(
            .data
            NAME1 = abc
            NAME2 = 2 )";
		std::istringstream iss(input);

		TokenList tl = tokenize(iss);
		Parse syntax;
		REQUIRE(syntax.parse(tl) == false);
	}

}



TEST_CASE("test .data -- label and layouts", "[parser]") {
	{
		std::string input = R"(
            .data
            x: .word -1, 2, +1

			.text
            )";
		std::istringstream iss(input);

		TokenList tl = tokenize(iss);
		Parse syntax;
		REQUIRE(syntax.parse(tl) == true);
	}

	{
		std::string input = R"(
            .data
            x: .word -1, 2,
            )";
		std::istringstream iss(input);

		TokenList tl = tokenize(iss);
		Parse syntax;
		REQUIRE(syntax.parse(tl) == false);
	}

	{
		std::string input = R"(
            .data
            x: .word
            )";
		std::istringstream iss(input);

		TokenList tl = tokenize(iss);
		Parse syntax;
		REQUIRE(syntax.parse(tl) == false);
	}

	{
		std::string input = R"(
            .data
            x!@#324: .word 1, 2, 3, 4, 5
            )";
		std::istringstream iss(input);

		TokenList tl = tokenize(iss);
		Parse syntax;
		REQUIRE(syntax.parse(tl) == false);
	}

	{
		std::string input = R"(
            .data
            abc!@# = 123
            )";
		std::istringstream iss(input);

		TokenList tl = tokenize(iss);
		Parse syntax;
		REQUIRE(syntax.parse(tl) == false);
	}

	{
		std::string input = R"(
            .data
            x: .word -1, -2, +3, -abc
            )";
		std::istringstream iss(input);

		TokenList tl = tokenize(iss);
		Parse syntax;
		REQUIRE(syntax.parse(tl) == false);
	}

	{
		std::string input = R"(
            .data
            x: .byte -1, -2, -993424242
            )";
		std::istringstream iss(input);

		TokenList tl = tokenize(iss);
		Parse syntax;
		REQUIRE(syntax.parse(tl) == false);
	}

	{
		std::string input = R"(
            .data
            x: .half -1, -2, -993424242342
            )";
		std::istringstream iss(input);

		TokenList tl = tokenize(iss);
		Parse syntax;
		REQUIRE(syntax.parse(tl) == false);
	}

	{
		std::string input = R"(
            .data
            x: .word -1, -2, 214748364734
            )";
		std::istringstream iss(input);

		TokenList tl = tokenize(iss);
		Parse syntax;
		REQUIRE(syntax.parse(tl) == false);
	}

	{
		std::string input = R"(
            .data
            x: .space -1, -2, 214748364744
            )";
		std::istringstream iss(input);

		TokenList tl = tokenize(iss);
		Parse syntax;
		REQUIRE(syntax.parse(tl) == false);
	}

	{
		std::string input = R"(
            .data
            x: .byte 99342424223
            )";
		std::istringstream iss(input);

		TokenList tl = tokenize(iss);
		Parse syntax;
		REQUIRE(syntax.parse(tl) == false);
	}

	{
		std::string input = R"(
            .data
            x: .half 993424242342
            )";
		std::istringstream iss(input);

		TokenList tl = tokenize(iss);
		Parse syntax;
		REQUIRE(syntax.parse(tl) == false);
	}

	{
		std::string input = R"(
            .data
            x: .word 214748364734
            )";
		std::istringstream iss(input);

		TokenList tl = tokenize(iss);
		Parse syntax;
		REQUIRE(syntax.parse(tl) == false);
	}

	{
		std::string input = R"(
            .data
            x: .space 214748364744
            )";
		std::istringstream iss(input);

		TokenList tl = tokenize(iss);
		Parse syntax;
		REQUIRE(syntax.parse(tl) == false);
	}

	{
		std::string input = R"(
            .data
            x: .byte -99342424223
            )";
		std::istringstream iss(input);

		TokenList tl = tokenize(iss);
		Parse syntax;
		REQUIRE(syntax.parse(tl) == false);
	}

	{
		std::string input = R"(
            .data
            x: .half -993424242342
            )";
		std::istringstream iss(input);

		TokenList tl = tokenize(iss);
		Parse syntax;
		REQUIRE(syntax.parse(tl) == false);
	}

	{
		std::string input = R"(
            .data
            x: .word -214748364734
            )";
		std::istringstream iss(input);

		TokenList tl = tokenize(iss);
		Parse syntax;
		REQUIRE(syntax.parse(tl) == false);
	}

	{
		std::string input = R"(
            .data
            x: .space -214748364744
            )";
		std::istringstream iss(input);

		TokenList tl = tokenize(iss);
		Parse syntax;
		REQUIRE(syntax.parse(tl) == false);
	}

	{
		std::string input = R"(
            .data
            x:
            )";
		std::istringstream iss(input);

		TokenList tl = tokenize(iss);
		Parse syntax;
		REQUIRE(syntax.parse(tl) == true);
	}

	{
		std::string input = R"(
        .data
    x:
		.text
	add $t0, $t1, $zero
	j

	
            )";
		std::istringstream iss(input);

		TokenList tl = tokenize(iss);
		Parse syntax;
		REQUIRE(syntax.parse(tl) == false);
	}

	{
		std::string input = R"(
            .data
            .word -1, 2
			.half -1, 3, +5
		y:	.byte 5, -3, +8
		x:	.space 70, -20, +90
            )";
		std::istringstream iss(input);

		TokenList tl = tokenize(iss);
		Parse syntax;
		REQUIRE(syntax.parse(tl) == true);
	}

	{
		std::string input = R"(
            .data
			y: .ascii "fsdkljsdlfjdsl &*%^^$%& :JKL:KJLHL"		# dflskjfdslkjflsdjf
            x: .asciiz "abc123 !@#$% defg"
			.text
			# glfkjlsdkjlf
            )";
		std::istringstream iss(input);

		TokenList tl = tokenize(iss);
		Parse syntax;
		REQUIRE(syntax.parse(tl) == true);
	}

	{
		std::string input = R"(
            .data
            12x: .asciiz "abc123 !@#$% defg"
            )";
		std::istringstream iss(input);

		TokenList tl = tokenize(iss);
		Parse syntax;
		REQUIRE(syntax.parse(tl) == false);
	}

	{
		std::string input = R"(
            .data
            x: .asciiz abc123 !@#$% defg"
            )";
		std::istringstream iss(input);

		TokenList tl = tokenize(iss);
		Parse syntax;
		REQUIRE(syntax.parse(tl) == false);
	}

	{
		std::string input = R"(
            .data
            x: .ascii abc123 !@#$% defg"
            )";
		std::istringstream iss(input);

		TokenList tl = tokenize(iss);
		Parse syntax;
		REQUIRE(syntax.parse(tl) == false);
	}

	{
		std::string input = R"(
            .data
            .asciiz abc123 !@#$% defg"
            )";
		std::istringstream iss(input);

		TokenList tl = tokenize(iss);
		Parse syntax;
		REQUIRE(syntax.parse(tl) == false);
	}

	{
		std::string input = R"(
            .data
            x: .asciiz "abc123 !@#$% defg"

			.text
		main:
			div $15, $16, main


            )";
		std::istringstream iss(input);

		TokenList tl = tokenize(iss);
		Parse syntax;
		REQUIRE(syntax.parse(tl) == false);
	}

}

TEST_CASE("Provided tests -- pass", "data section") {
	{
		std::string input = R"(
	# A test file of data declarations only
	.data
var1:	.word 1024             # int var1 = 1024

var2:	.half 12               # short var2 = 12
	
var3:	.byte 0                # char var3 = 0

var4:	.byte 1, 2, 3, 4, 5, 6, 7, 8  # var4 = {1,2,3,4,5,6,7,8}

var5:	.space 512             # reserve 512 bytes

var6:	.ascii "hello"

var7:	.asciiz "goodbye"

	.text
	

            )";
		std::istringstream iss(input);

		TokenList tl = tokenize(iss);
		Parse syntax;
		REQUIRE(syntax.parse(tl) == true);
	}

	{
		std::string input = R"(
	# a test for constants
	.data
	LENGTH = 1024
arr:	.space LENGTH
	
	.text

            )";
		std::istringstream iss(input);

		TokenList tl = tokenize(iss);
		Parse syntax;
		REQUIRE(syntax.parse(tl) == true);
	}
}

TEST_CASE("Provided tests -- fail", "data section") {
	{
		std::string input = R"(	
# A test file of data declarations only
	.data
var1:	.word 1024             # int var1 = 1024

var2:	.half 12               # short var2 = 12
	
var3:	.byte 0                # char var3 = 0

var4:	.byte 1, 2, 3, 4, 5, 6, 7,   # PARSE ERROR

var5:	.space 512             # reserve 512 bytes

var6:	.ascii "hello"

var7:	.asciiz "goodbye"

	.text
	
)";
		std::istringstream iss(input);

		TokenList tl = tokenize(iss);
		Parse syntax;
		REQUIRE(syntax.parse(tl) == false);


	}

	{
		std::string input = R"(
        # A test file of data declarations only
	    .data
        var1:	.byte           # PARSE ERROR
		)";
		std::istringstream iss(input);

		TokenList tl = tokenize(iss);
		Parse syntax;
		REQUIRE(syntax.parse(tl) == false);
	}

	{
		std::string input = R"(
        # A test file of data declarations only
	    .data
        var1:	.word           # PARSE ERROR
		)";
		std::istringstream iss(input);

		TokenList tl = tokenize(iss);
		Parse syntax;
		REQUIRE(syntax.parse(tl) == false);
	}

	{
		std::string input = R"(
        # A test file of data declarations only
	    .data
        var1:	.half           # PARSE ERROR
		)";
		std::istringstream iss(input);

		TokenList tl = tokenize(iss);
		Parse syntax;
		REQUIRE(syntax.parse(tl) == false);
	}

	{
		std::string input = R"(
        # A test file of data declarations only
	    .data
        var1:	.space           # PARSE ERROR
		)";
		std::istringstream iss(input);

		TokenList tl = tokenize(iss);
		Parse syntax;
		REQUIRE(syntax.parse(tl) == false);
	}

	{
		std::string input = R"(
        # A test file of data declarations only
	    .data
        var1:	.space  "hello"         # PARSE ERROR

		.text
		)";
		std::istringstream iss(input);

		TokenList tl = tokenize(iss);
		Parse syntax;
		REQUIRE(syntax.parse(tl) == false);
	}

	{
		std::string input = R"(
        # A test file of data declarations only
	    .data
        var1:	.ascii  34         # PARSE ERROR

		.text
		)";
		std::istringstream iss(input);

		TokenList tl = tokenize(iss);
		Parse syntax;
		REQUIRE(syntax.parse(tl) == false);
	}

	{
		std::string input = R"(
        # A test file of data declarations only
	    .data
        var1:	.asciiz  34         # PARSE ERROR

		.text
		)";
		std::istringstream iss(input);

		TokenList tl = tokenize(iss);
		Parse syntax;
		REQUIRE(syntax.parse(tl) == false);
	}

	{
		std::string input = R"(
        # A test file of data declarations only
	    .data
        12321dfsfs:	.space  3         # PARSE ERROR
		)";
		std::istringstream iss(input);

		TokenList tl = tokenize(iss);
		Parse syntax;
		REQUIRE(syntax.parse(tl) == false);
	}

	{
		std::string input = R"(
        # A test file of data declarations only
	    .data
        a:	.ascii  "sfsfds         # PARSE ERROR
		)";
		std::istringstream iss(input);

		TokenList tl = tokenize(iss);
		Parse syntax;
		REQUIRE(syntax.parse(tl) == false);
	}

	{
		std::string input = R"(
        # A test file of data declarations only
	    .data
        var1:	.word 5,            # PARSE ERROR
		)";
		std::istringstream iss(input);

		TokenList tl = tokenize(iss);
		Parse syntax;
		REQUIRE(syntax.parse(tl) == false);
	}
	{
		std::string input = R"(
        # A test file of data declarations only
	    .data
        var1:	.byte -6, 5,           # PARSE ERROR
		)";
		std::istringstream iss(input);

		TokenList tl = tokenize(iss);
		Parse syntax;
		REQUIRE(syntax.parse(tl) == false);
	}

	{
		std::string input = R"(
        # A test file of data declarations only
	    .data
        var1 = abc         # PARSE ERROR

		.text
		)";
		std::istringstream iss(input);

		TokenList tl = tokenize(iss);
		Parse syntax;
		REQUIRE(syntax.parse(tl) == false);
	}

	{
		std::string input = R"(
        # A test file of data declarations only
	    .data
        var1 = 1, 2, 3         # PARSE ERROR

		.text
		)";
		std::istringstream iss(input);

		TokenList tl = tokenize(iss);
		Parse syntax;
		REQUIRE(syntax.parse(tl) == false);
	}

	{
		std::string input = R"(
        # A test file of data declarations only
	    .data
        123 = 123         # PARSE ERROR

		.text
		)";
		std::istringstream iss(input);

		TokenList tl = tokenize(iss);
		Parse syntax;
		REQUIRE(syntax.parse(tl) == false);
	}

	{
		std::string input = R"(
        # A test file of data declarations only
	    .data
        x:	.abc 1, 2, 3, 4         # PARSE ERROR

		.text
		)";
		std::istringstream iss(input);

		TokenList tl = tokenize(iss);
		Parse syntax;
		REQUIRE(syntax.parse(tl) == false);
	}
}


TEST_CASE("Provided tests -- pass -- text section") {


	{
		std::string input = R"(
	.data
avar:	.word 	0
bvar:	.half	1   #fdlksjflsd fkdlsjflsdj fsdf
cvar:	.byte	3

	.text
main:	
    mfhi $s1
    mflo $t0    # ksld jfls dsf 
    mthi $21    #rlodwjflsjflsf
    mtlo $9    #dslkjfsdfdsf fdslfjk )";
		std::istringstream iss(input);

		TokenList tl = tokenize(iss);
		Parse syntax;
		REQUIRE(syntax.parse(tl) == true);
	}

	{
		std::string input = R"(
	# a test of basic arithmetic operations
	.data
x:	.word	1
y:	.word	1
z:	.space 	4

	.text
main:
	lw 	$t0, x
	lw 	$t1, y
	add 	$t2, $t0, $t1
	add 	$t2, $t0, 2
	addu 	$t2, $t0, $t1
	addu 	$t2, $t0, 2
	sub 	$t2, $t0, $t1
	sub 	$t2, $t0, 2
	subu 	$t2, $t0, $t1
	subu 	$t2, $t0, 2
	mul 	$t2, $t0, $t1
	mul 	$t2, $t0, 2
	mulo 	$t2, $t0, $t1
	mulo 	$t2, $t0, 2
	mulou 	$t2, $t0, $t1
	mulou 	$t2, $t0, 2
	mult	$t0, $t1
	multu	$t0, $t1
	div 	$t2, $t0, $t1
	div 	$t2, $t0, 2         #kldsjfllsdjfldsjf
	divu 	$t2, $t0, $t1
	divu 	$t2, $t0, 2
	div		$t0, $t1
	divu	$t0, $t1
	rem 	$t2, $t0, $t1
	rem 	$t2, $t0, 2
	remu 	$t2, $t0, $t1
	remu 	$t2, $t0, 2
	abs		$t0, $t1
	neg		$t0, $t1
	negu	$t0, $t1
	
    #fdlskjfdlksjf slkdjflsdf
)";
		std::istringstream iss(input);

		TokenList tl = tokenize(iss);
		Parse syntax;
		REQUIRE(syntax.parse(tl) == true);
	}

	{
		std::string input = R"(
	# test of the basic control instructions
	.data
x:	.word 1
y:	.word 2

	.text
main:
	lw $t0, x
	lw $t1, y
	
	beq $t0, $t0, next1
next1:

	bne $t0, $t1, next2
next2:

	blt $t0, $t1, next3
next3:

	ble $t0, $t0, next4
next4:

	bgt $t1, $t0, next5
next5:

	bge $t0, $t0, next6
next6:
	
end:
	j	end
 )";
		std::istringstream iss(input);

		TokenList tl = tokenize(iss);
		Parse syntax;
		REQUIRE(syntax.parse(tl) == true);
	}

	{
		std::string input = R"(
	# Example program to compute the sum of squares from Jorgensen [2016]

	#---------------------------------------------------------------
	# data declarations
	
	.data
n:		.word 10
sumOfSquares:	.word 0

	#---------------------------------------------------------------
	# the program
	.text
main:
	lw $t0,n
	li $t1,1
	li $t2,0
    

sumLoop:
	mul $t3, $t1, $t1
	add $t2, $t2, $t3
	add $t1, $t1, 1
	ble $t1, $t0, sumLoop
	sw  $t2, sumOfSquares
end:
	j end
)";
		std::istringstream iss(input);

		TokenList tl = tokenize(iss);
		Parse syntax;
		REQUIRE(syntax.parse(tl) == true);
	}

	{
		std::string input = R"(
	# test of basic logical instructions
	.data
	TRUE = 1
	FALSE = 0

test1:	.word TRUE
test2:	.word FALSE
	
	.text
main:
	lw	$t0, test1
	lw	$t1, test2
	
	and	$t2, $t0, $t1
	and	$t2, $t0, TRUE
	nor	$t2, $t0, $t1
	nor	$t2, $t0, TRUE
	not	$t2, $t0
	not	$t2, $t0
	or	$t2, $t0, $t1
	or	$t2, $t0, TRUE
	xor	$t2, $t0, $t1
	xor	$t2, $t0, TRUE
	

)";
		std::istringstream iss(input);

		TokenList tl = tokenize(iss);
		Parse syntax;
		REQUIRE(syntax.parse(tl) == true);
	}

	{
		std::string input = R"(
	# a test of address modes
	.data
str:	.ascii	"hello"

	.text
main:
	lb $t8, str
	la $t0, str
	lb $t1, ($t0)
	lb $t2, 1($t0)
	lb $t3, 2($t0)
	lb $t4, 3($t0)
	lb $t5, 4($t0)
	
end:	nop
	j end
	
)";
		std::istringstream iss(input);

		TokenList tl = tokenize(iss);
		Parse syntax;
		REQUIRE(syntax.parse(tl) == true);
	}

	{
		std::string input = R"(
	# a test of all register names
	.data

	.text
main:	sw $1, $0
	
	sw $at, $zero
	sw $2, $gp
	sw $v0, $zero
	sw $3, $0
	sw $v1, $zero
	sw $4, $0
	sw $a0, $zero
	sw $5, $0
	nop
	sw $a1, $zero
	sw $6, $sp
	sw $a2, $zero
	sw $7, $0
	sw $a3, $zero
	sw $8, $0
	sw $t0, $zero
	sw $9, $0
	sw $t1, $zero
	sw $10, $fp
	sw $t2, $zero
	sw $11, $0
	sw $t3, $zero
	sw $12, $ra
	sw $t4, $zero
	sw $13, $0
	sw $t5, $zero
	sw $14, $at
	sw $t6, $zero
	sw $15, $0
	sw $t7, $zero
	sw $16, $0
	sw $s0, $zero
	sw $17, $0
	sw $s1, $zero
	sw $18, $0
	sw $s2, $zero
	sw $19, $0
	sw $s3, $zero
	sw $20, $0
	sw $s4, $zero
	sw $21, $0
	sw $s5, $zero
	sw $22, $0
	sw $s6, $zero
	sw $23, $0
	sw $s7, $zero
	sw $24, $0
	sw $t8, $zero
	sw $25, $0
	sw $t9, $zero
	sw $26, $0
	sw $k0, $zero
	sw $27, $0
	sw $k1, $zero
	sw $28, $0
	sw $gp, $zero
	sw $29, $0
	sw $sp, $zero
	sw $30, $0
	sw $fp, $zero
	sw $31, $0
	sw $ra, $zero

end:
	j end


)";
		std::istringstream iss(input);

		TokenList tl = tokenize(iss);
		Parse syntax;
		REQUIRE(syntax.parse(tl) == true);
	}

	{
		std::string input = R"(
	.data
var:	.word 0
	.word 1
	.word 2
x = 10
	.text
	
	# lexer error line 10 
main:	la $t0, var 
		lw $s1, x($t0)
	
                )";
		std::istringstream iss(input);

		TokenList tl = tokenize(iss);
		Parse syntax;
		REQUIRE(syntax.parse(tl) == true);
	}
}

TEST_CASE("Provided tests -- fail -- text section") {
	{
		std::string input = R"(
	.data
var:	.word 0
	.word 1
	.word 2

	.text
	
	# lexer error line 10 
main:	la $t0, var 
	lw $s1, ($t0
	
                )";
		std::istringstream iss(input);

		TokenList tl = tokenize(iss);
		Parse syntax;
		REQUIRE(syntax.parse(tl) == false);
	}

	{
		std::string input = R"(
	.data
var:	.word 0
	.word 1
	.word 2

	.text
	
	# lexer error line 10 
main:	la $t0, var 
		lw $s1, ($t0)
		j end
	
                )";
		std::istringstream iss(input);

		TokenList tl = tokenize(iss);
		Parse syntax;
		REQUIRE(syntax.parse(tl) == false);
	}

	{
		std::string input = R"(
	.data
var:	.word 0
	.word 1
	.word 2

	.text
	
	# lexer error line 10 
main:	la $t0, var 
		lw $s1, ($t0)
		li x, $5			# PARSE ERROR
	
                )";
		std::istringstream iss(input);

		TokenList tl = tokenize(iss);
		Parse syntax;
		REQUIRE(syntax.parse(tl) == false);
	}

	{
		std::string input = R"(
	.data
var:	.word 0
	.word 1
	.word 2

	.text
	
	# lexer error line 10 
main:	la $t0, var 
		lw $s1, var($t0)
		
	
                )";
		std::istringstream iss(input);

		TokenList tl = tokenize(iss);
		Parse syntax;
		REQUIRE(syntax.parse(tl) == false);
	}

	{
		std::string input = R"(
	.data
	TRUE = 1
	FALSE = 0

  test1:	.word TRUE
  test2:	.word TRUE
	
	.text
main:
  xor	TRUE, $t2, $t1

                )";
		std::istringstream iss(input);

		TokenList tl = tokenize(iss);
		Parse syntax;
		REQUIRE(syntax.parse(tl) == false);
	}

	{
		std::string input = R"(
	.data
	TRUE = 1
	FALSE = 0

  test1:	.word TRUE
  test2:	.word TRUE
	
	.text
main:
  xor	$t0, test1, $t1

                )";
		std::istringstream iss(input);

		TokenList tl = tokenize(iss);
		Parse syntax;
		REQUIRE(syntax.parse(tl) == false);
	}

	{
		std::string input = R"(
	.data
	TRUE = 1
	FALSE = 0

  test1:	.word TRUE
  test2:	.word TRUE
	
	.text
main:
  or	$t0, $t2, $t1

                )";
		std::istringstream iss(input);

		TokenList tl = tokenize(iss);
		Parse syntax;
		REQUIRE(syntax.parse(tl) == true);
	}

	{
		std::string input = R"(
	.data
	TRUE = 1
	FALSE = 0

  test1:	.word TRUE
  test2:	.word TRUE
	
	.text
main:
  abc	$t0, $t2, $t1

                )";
		std::istringstream iss(input);

		TokenList tl = tokenize(iss);
		Parse syntax;
		REQUIRE(syntax.parse(tl) == false);
	}

	{
		std::string input = R"(
	.data
	.text
		
	lw	$s0, $s1(5)

                )";
		std::istringstream iss(input);

		TokenList tl = tokenize(iss);
		Parse syntax;
		REQUIRE(syntax.parse(tl) == false);
	}

	{
		std::string input = R"(
	.data

	.text
main:
  lb	var, $t2
  

                )";
		std::istringstream iss(input);

		TokenList tl = tokenize(iss);
		Parse syntax;
		REQUIRE(syntax.parse(tl) == false);
	}

	{
		std::string input = R"(
	.data

	.text
main:
  lh	$t1, $t2
  

                )";
		std::istringstream iss(input);

		TokenList tl = tokenize(iss);
		Parse syntax;
		REQUIRE(syntax.parse(tl) == true);
	}

	{
		std::string input = R"(
	.data

	.text
main:
  la	$t1, $t2
)";
		std::istringstream iss(input);

		TokenList tl = tokenize(iss);
		Parse syntax;
		REQUIRE(syntax.parse(tl) == true);
		REQUIRE(syntax.getLine() == 6);
	}

	{
		std::string input = R"(
	.data
avar:	.word 	0
bvar:	.half	1   #fdlksjflsd fkdlsjflsdj fsdf
cvar:	.byte	3

	.text
main:	
    mfhi 
    mflo $t0    # ksld jfls dsf 
    mthi $21
    mtlo $9, $10 
)";
		std::istringstream iss(input);

		TokenList tl = tokenize(iss);
		Parse syntax;
		REQUIRE(syntax.parse(tl) == false);
	}

	{
		std::string input = R"(
	.data
	.text
main:
	bgt	$t2, $t2, var 
)";
		std::istringstream iss(input);

		TokenList tl = tokenize(iss);
		Parse syntax;
		REQUIRE(syntax.parse(tl) == false);
	}

	{
		std::string input = R"(

main:
	bgt	$t2, $t2, var 
)";
		std::istringstream iss(input);

		TokenList tl = tokenize(iss);
		Parse syntax;
		REQUIRE(syntax.parse(tl) == false);
	}

	{
		std::string input = R"(
	.data
	.text
main
	bgt	$t2, $t2, var 
)";
		std::istringstream iss(input);

		TokenList tl = tokenize(iss);
		Parse syntax;
		REQUIRE(syntax.parse(tl) == false);
	}

	{
		std::string input = R"(
	# a test for basic data movement
	.data
avar:	.word 	0
bvar:	.half	1
cvar:	.byte	3

	.text
main:	
	li $t0, $t1 # Parse Error
	lw $t1, avar
	lh $t2, bvar
	lb $t2, cvar
	sw $t1, avar
	sh $t2, bvar
	sb $t2, cvar

	move $t0, $0
	

)";
		std::istringstream iss(input);

		TokenList tl = tokenize(iss);
		Parse syntax;
		REQUIRE(syntax.parse(tl) == false);
	}

	{
		std::string input = R"(
	# a test of basic arithmetic operations
	.data
x:	.word	1
y:	.word	1
z:	.space 	4

	.text
main:
	lw 	$t0, x
	lw 	$t1, y
	add 	$t2, $t0, $t1
	add 	$t2, $t0, 2
	addu 	$t2, $t0, $t1
	addu 	$t2, $t0, 2
	sub 	$t2, $t0, $t1
	sub 	$t2, $t0, 2
	subu 	$t2, $t0, $t1
	subu 	$t2, $t0, 2
	mul 	$t2, $t0, $t1
	mul 	$t2, $t0, 2
	mulo 	$t2, $t0, $t1
	mulo 	$t2, $t0, 2
	mulou 	$t2, $t0, $t1
	mulou 	$t2, $t0, 2
	mult	$t0, $t1
	multu	$t0, $t1
	div 	$t2, $t0, $t1
	div 	$t2, $t0, 2
	divu 	$t2, $t0, $t1
	divu 	$t2, $t0, 2
	div	$t0, $t1
	divu	$t0, $t1
	rem 	$t2, $t0, $t1
	rem 	$t2, $t0	# parse error
	remu 	$t2, $t0, $t1
	remu 	$t2, $t0, 2
	abs	$t0, $t1
	neg	$t0, $t1
	negu	$t0, $t1
	

)";
		std::istringstream iss(input);

		TokenList tl = tokenize(iss);
		Parse syntax;
		REQUIRE(syntax.parse(tl) == false);
	}

	{
		std::string input = R"(
	# test of basic logical instructions
	.data
	TRUE = 1
	FALSE = 0

test1:	.word TRUE
test2:	.word FALSE
	
	.text
main:
	lw	$t0, test1
	lw	$t1, test2
	
	and	$t2, $t0, $t1
	and	$t2, $t0, TRUE
	nor	$t2, $t0, $t1
	nor	$t2, $t0, TRUE, FALSE # parse error
	not	$t2, $t0
	not	$t2, $t0
	or	$t2, $t0, $t1
	or	$t2, $t0, TRUE
	xor	$t2, $t0, $t1
	xor	$t2, $t0, TRUE
	
)";
		std::istringstream iss(input);

		TokenList tl = tokenize(iss);
		Parse syntax;
		REQUIRE(syntax.parse(tl) == false);
	}

	{
		std::string input = R"(
	# test of the basic control instructions
	.data
x:	.word 1
y:	.word 2

	.text
main:
	lw $t0, x
	lw $t1, y
	
	beq $t0, $t0, next1
next1:

	bne $t0, $t1, next2
next2:

	blt $t0, $t1, next3
next3:

	ble $t0, $t0, next4
next4:

	bgt $t1, next5      # parse error
next5:

	bge $t0, $t0, next6
next6:
	
end:
	j end

)";
		std::istringstream iss(input);

		TokenList tl = tokenize(iss);
		Parse syntax;
		REQUIRE(syntax.parse(tl) == false);
	}

	{
		std::string input = R"(
	# Example program to compute the sum of squares from Jorgensen [2016]

	#---------------------------------------------------------------
	# data declarations
	
	.data
n:		.word 10
sumOfSquares:	.word 0

	#---------------------------------------------------------------
	# the program
	.text
main:
	lw $t0,n
	li $t1,1
	li $t2,0

sumLoop:
	mul $t3, $t1, $t1
	add $t2, $t2, $32	# parse error
	add $t1, $t1, 1
	ble $t1, $t0, sumLoop
	sw  $t2, sumOfSquares

end:
	j end
	

)";
		std::istringstream iss(input);

		TokenList tl = tokenize(iss);
		Parse syntax;
		REQUIRE(syntax.parse(tl) == false);
	}

	{
		// no .text
		std::string input = R"(
	# Example program to compute the sum of squares from Jorgensen [2016]

	#---------------------------------------------------------------
	# data declarations
	
	.data
n:		.word 10
sumOfSquares:	.word 0

	#---------------------------------------------------------------
	# the program
main:
	lw $t0,n
	li $t1,1
	li $t2,0

sumLoop:
	mul $t3, $t1, $t1
	add $t2, $t2, $t3
	add $t1, $t1, 1
	ble $t1, $t0, sumLoop
	sw  $t2, sumOfSquares

end:
	j end
	

)";
		std::istringstream iss(input);

		TokenList tl = tokenize(iss);
		Parse syntax;
		REQUIRE(syntax.parse(tl) == false);
	}

}

// xxxxxxxxxxxxxxxxxxxxxxxxxxxxxxx   test VM below   xxxxxxxxxxxxxxxxxxxxxxxxxxx

TEST_CASE("vm00") {
	{
		std::string file = R"(# simple infinite loop
	.data
	.text
main:	j main
)";
		std::stringstream ss(file);
		TokenList tl = tokenize(ss);
		Parse syntax;
		syntax.parse(tl);
		VirtualMachine VM = syntax.getVM();

		for (int i = 0; i < 31; i++) {
			uint32_t value = VM.readReg(i);
			REQUIRE(value == 0);
		}
		REQUIRE(VM.readPC() == 0);
		REQUIRE(VM.readHI() == 0);
		REQUIRE(VM.readLO() == 0);

		VM.simulation();
		REQUIRE(VM.readPC() == 0);
		REQUIRE(VM.getStatus() == VM_Status::Simulating);
	}

}


TEST_CASE("vm01") {
	std::string file = R"(
        .data
        .space 8
var1:   .word 1
var2:   .word -2

        .text
main:
     	la $t0, var1

	lw $t1, 0
	lw $t2, $t0
	lw $t3, 4($t0)
	lw $t4, 4(var1)
	lw $t5, var2
	
)";
	std::stringstream ss(file);
	TokenList tl = tokenize(ss);
	Parse syntax;
	syntax.parse(tl);
	VirtualMachine VM = syntax.getVM();

	REQUIRE(VM.readPC() == 0);
	REQUIRE(VM.readMEM(8, 1) == 0x01);
	REQUIRE(VM.readMEM(9, 1) == 0x00);
	REQUIRE(VM.readMEM(10, 1) == 0x00);
	REQUIRE(VM.readMEM(11, 1) == 0x00);
	REQUIRE(VM.readMEM(12, 1) == 0xfe);
	REQUIRE(VM.readMEM(13, 1) == 0xff);
	REQUIRE(VM.readMEM(14, 1) == 0xff);
	REQUIRE(VM.readMEM(15, 1) == 0xff);

	VM.simulation();
	REQUIRE(VM.readPC() == 0x00000001);
	REQUIRE(VM.readReg(8) == 8);

	VM.simulation();
	REQUIRE(VM.readPC() == 0x00000002);
	REQUIRE(VM.readReg(9) == 0);

	VM.simulation();
	REQUIRE(VM.readPC() == 0x00000003);
	REQUIRE(VM.readReg(10) == 1);

	VM.simulation();
	REQUIRE(VM.readPC() == 0x00000004);
	REQUIRE(VM.readReg(11) == 0xfffffffe);

	VM.simulation();
	REQUIRE(VM.readPC() == 0x00000005);
	REQUIRE(VM.readReg(12) == 0xfffffffe);

	VM.simulation();
	REQUIRE(VM.readPC() == 0x00000006);
	REQUIRE(VM.readReg(13) == 0xfffffffe);
}


TEST_CASE("vm02") {

	std::string file = R"(
        .data
r1:     .space 4
r2:     .space 12
r3:     .space 4
var:    .word 7

        .text
main:
        la $t0, r2
     	lw $t1, var

	sw $t1, 0
	sw $t1, $t0
	sw $t1, 4($t0)
	sw $t1, 8(r2)
	sw $t1, r3
	
)";
	std::stringstream ss(file);
	TokenList tl = tokenize(ss);
	Parse syntax;
	syntax.parse(tl);
	VirtualMachine VM = syntax.getVM();
	REQUIRE(VM.readPC() == 0x00000000);

	VM.simulation();
	REQUIRE(VM.readPC() == 0x00000001);
	REQUIRE(VM.readReg(8) == 4);

	VM.simulation();
	REQUIRE(VM.readPC() == 0x00000002);
	REQUIRE(VM.readReg(9) == 7);

	VM.simulation();
	REQUIRE(VM.readPC() == 0x00000003);
	REQUIRE(VM.readMEM(0, 1) == 0x07);
	REQUIRE(VM.readMEM(1, 1) == 0x00);
	REQUIRE(VM.readMEM(2, 1) == 0x00);
	REQUIRE(VM.readMEM(3, 1) == 0x00);

	VM.simulation();
	REQUIRE(VM.readPC() == 0x00000004);
	REQUIRE(VM.readMEM(4, 1) == 0x07);
	REQUIRE(VM.readMEM(5, 1) == 0x00);
	REQUIRE(VM.readMEM(6, 1) == 0x00);
	REQUIRE(VM.readMEM(7, 1) == 0x00);

	VM.simulation();
	REQUIRE(VM.readPC() == 0x00000005);
	REQUIRE(VM.readMEM(8, 1) == 0x07);
	REQUIRE(VM.readMEM(9, 1) == 0x00);
	REQUIRE(VM.readMEM(10, 1) == 0x00);
	REQUIRE(VM.readMEM(11, 1) == 0x00);

	VM.simulation();
	REQUIRE(VM.readPC() == 0x00000006);
	REQUIRE(VM.readMEM(12, 1) == 0x07);
	REQUIRE(VM.readMEM(13, 1) == 0x00);
	REQUIRE(VM.readMEM(14, 1) == 0x00);
	REQUIRE(VM.readMEM(15, 1) == 0x00);

	VM.simulation();
	REQUIRE(VM.readPC() == 0x00000007);
	REQUIRE(VM.readMEM(16, 1) == 0x07);
	REQUIRE(VM.readMEM(17, 1) == 0x00);
	REQUIRE(VM.readMEM(18, 1) == 0x00);
	REQUIRE(VM.readMEM(19, 1) == 0x00);

}


TEST_CASE("vm03") {

	std::string file = R"(
        .data
A = 0
B = 1
C = 2
D = 4
        .text
main:
        li $t0, 100
        li $t1, A
        li $t2, B
        li $t3, C
        li $t4, D
	
)";
	std::stringstream ss(file);
	TokenList tl = tokenize(ss);
	Parse syntax;
	syntax.parse(tl);
	VirtualMachine VM = syntax.getVM();
	REQUIRE(VM.readPC() == 0x00000000);

	VM.simulation();
	REQUIRE(VM.readPC() == 0x00000001);
	REQUIRE(VM.readReg(8) == 0x00000064);

	VM.simulation();
	REQUIRE(VM.readReg(9) == 0x00000000);

	VM.simulation();
	REQUIRE(VM.readReg(10) == 0x00000001);

	VM.simulation();
	REQUIRE(VM.readReg(11) == 0x00000002);

	VM.simulation();
	REQUIRE(VM.readReg(12) == 0x00000004);
}


TEST_CASE("vm04") {

	std::string file = R"(
        .data
VALUE = -1234

        .text
main:
        li $t0, VALUE
        move $t1, $t0
        move $t2, $t1
        move $t3, $t2
        move $t4, $t3
        move $t5, $t4
        move $t6, $t5
        move $t7, $t6
	
)";
	std::stringstream ss(file);
	TokenList tl = tokenize(ss);
	Parse syntax;
	syntax.parse(tl);
	VirtualMachine VM = syntax.getVM();
	REQUIRE(VM.readPC() == 0x00000000);

	VM.simulation();
	REQUIRE(VM.readPC() == 0x00000001);
	REQUIRE(VM.readReg(8) == 0xfffffb2e);
	REQUIRE(VM.readReg(9) == 0x00000000);
	REQUIRE(VM.readReg(14) == 0x00000000);
	REQUIRE(VM.readReg(15) == 0x00000000);

	VM.simulation();
	REQUIRE(VM.readReg(9) == 0xfffffb2e);

	VM.simulation();
	REQUIRE(VM.readReg(10) == 0xfffffb2e);

	VM.simulation();
	REQUIRE(VM.readReg(11) == 0xfffffb2e);

	VM.simulation();
	REQUIRE(VM.readReg(12) == 0xfffffb2e);

	VM.simulation();
	REQUIRE(VM.readReg(13) == 0xfffffb2e);

	VM.simulation();
	REQUIRE(VM.readReg(14) == 0xfffffb2e);

	VM.simulation();
	REQUIRE(VM.readReg(15) == 0xfffffb2e);
}

TEST_CASE("vm05") {

	std::string file = R"(
        .data
VALUE = -1
var:    .word 1
        .text
main:
        lw $t0, var
        add $t1, $t0, VALUE
        add $t2, $t1, $t0
	
)";
	std::stringstream ss(file);
	TokenList tl = tokenize(ss);
	Parse syntax;
	syntax.parse(tl);
	VirtualMachine VM = syntax.getVM();
	REQUIRE(VM.readPC() == 0x00000000);

	VM.simulation();
	VM.simulation();
	VM.simulation();
	REQUIRE(VM.readPC() == 0x00000003);
	REQUIRE(VM.readReg(8) == 0x00000001);
	REQUIRE(VM.readReg(9) == 0x00000000);
	REQUIRE(VM.readReg(10) == 0x00000001);

}

TEST_CASE("vm06") {

	std::string file = R"(
        .data
VALUE = 12
var:    .word 31
        .text
main:
        lw $t0, var
        addu $t1, $t0, VALUE # 31+12=43
        addu $t2, $t1, $t0 # 43+31=74
	
)";
	std::stringstream ss(file);
	TokenList tl = tokenize(ss);
	Parse syntax;
	syntax.parse(tl);
	VirtualMachine VM = syntax.getVM();
	REQUIRE(VM.readPC() == 0x00000000);

	VM.simulation();
	VM.simulation();
	VM.simulation();
	REQUIRE(VM.readPC() == 0x00000003);
	REQUIRE(VM.readReg(8) == 0x0000001f);
	REQUIRE(VM.readReg(9) == 0x0000002b);
	REQUIRE(VM.readReg(10) == 0x0000004a);

}

TEST_CASE("vm07") {

	std::string file = R"(
        .data
VALUE = 2
var1:   .word 1
var2:   .word 12
var3:   .word -1
        .text
main:
        lw $t0, var1
        lw $t1, var2
        lw $t2, var3
        sub $t3, $t0, VALUE # 1-2 = -1
        sub $t4, $t1, $t0 # 12-1 = 11
        sub $t5, $t2, VALUE # -1-2 = -3

)";
	std::stringstream ss(file);
	TokenList tl = tokenize(ss);
	Parse syntax;
	syntax.parse(tl);
	VirtualMachine VM = syntax.getVM();
	REQUIRE(VM.readPC() == 0x00000000);

	VM.simulation();
	VM.simulation();
	VM.simulation();
	REQUIRE(VM.readPC() == 0x00000003);
	REQUIRE(VM.readReg(8) == 0x00000001);
	REQUIRE(VM.readReg(9) == 0x0000000c);
	REQUIRE(VM.readReg(10) == 0xffffffff);

	VM.simulation();
	VM.simulation();
	VM.simulation();
	REQUIRE(VM.readPC() == 0x00000006);
	REQUIRE(VM.readReg(11) == 0xffffffff);
	REQUIRE(VM.readReg(12) == 0x0000000b);
	REQUIRE(VM.readReg(13) == 0xfffffffd);
}

TEST_CASE("vm08") {

	std::string file = R"(
        .data
VALUE = 2
var1:   .word 1
var2:   .word 12
var3:   .word -1
        .text
main:
        lw $t0, var1
        lw $t1, var2
        lw $t2, var3
        subu $t3, $t0, VALUE # 1-2 = -1 = 4294967295
        subu $t4, $t1, $t0 # 12-1 = 11
        subu $t5, $t2, VALUE # -1-2 = -3 = 4294967293
	
)";
	std::stringstream ss(file);
	TokenList tl = tokenize(ss);
	Parse syntax;
	syntax.parse(tl);
	VirtualMachine VM = syntax.getVM();
	REQUIRE(VM.readPC() == 0x00000000);

	VM.simulation();
	VM.simulation();
	VM.simulation();
	REQUIRE(VM.readPC() == 0x00000003);
	REQUIRE(VM.readReg(8) == 0x00000001);
	REQUIRE(VM.readReg(9) == 0x0000000c);
	REQUIRE(VM.readReg(10) == 0xffffffff);

	VM.simulation();
	VM.simulation();
	VM.simulation();
	REQUIRE(VM.readPC() == 0x00000006);
	REQUIRE(VM.readReg(11) == 0xffffffff);
	REQUIRE(VM.readReg(12) == 0x0000000b);
	REQUIRE(VM.readReg(13) == 0xfffffffd);
}

TEST_CASE("vm09") {

	std::string file = R"(
        .data
VALUE = 4
VALUE2 = -4
var1:   .word 2
var2:   .word -2
var3:   .word 1073741824 # = 2^30
        .text
main:
        lw $t0, var1
        li $t1, VALUE
        mult $t0, $t1 # 2*4 = 8
        mflo $t8
        mfhi $t9

        lw $t0, var2
        lw $t1, var1
        mult $t0, $t1 # -2*2 = -4 
        mflo $t8
        mfhi $t9

        lw $t0, var3
        li $t1, VALUE
        mult $t0, $t1 # 1073741824*4 = 4294967296 (overflow)
        mflo $t8
        mfhi $t9

        lw $t0, var3
        li $t1, VALUE2
        mult $t0, $t1 # 1073741824*-4 = -4294967296 (overflow)
        mflo $t8
        mfhi $t9	
)";
	std::stringstream ss(file);
	TokenList tl = tokenize(ss);
	Parse syntax;
	syntax.parse(tl);
	VirtualMachine VM = syntax.getVM();
	REQUIRE(VM.readPC() == 0x00000000);

	VM.simulation();
	VM.simulation();
	VM.simulation();
	VM.simulation();
	VM.simulation();
	REQUIRE(VM.readPC() == 0x00000005);
	REQUIRE(VM.readReg(8) == 0x00000002);
	REQUIRE(VM.readReg(9) == 0x00000004);
	REQUIRE(VM.readLO() == 0x00000008);
	REQUIRE(VM.readHI() == 0x00000000);
	REQUIRE(VM.readReg(24) == 0x00000008);
	REQUIRE(VM.readReg(25) == 0x00000000);

	VM.simulation();
	VM.simulation();
	VM.simulation();
	VM.simulation();
	VM.simulation();
	REQUIRE(VM.readPC() == 0x0000000a);
	REQUIRE(VM.readReg(8) == 0xfffffffe);
	REQUIRE(VM.readReg(9) == 0x00000002);
	REQUIRE(VM.readLO() == 0xfffffffc);
	REQUIRE(VM.readHI() == 0xffffffff);
	REQUIRE(VM.readReg(24) == 0xfffffffc);
	REQUIRE(VM.readReg(25) == 0xffffffff);

	VM.simulation();
	VM.simulation();
	VM.simulation();
	VM.simulation();
	VM.simulation();
	REQUIRE(VM.readPC() == 0x0000000f);
	REQUIRE(VM.readReg(8) == 0x40000000);
	REQUIRE(VM.readReg(9) == 0x00000004);
	REQUIRE(VM.readLO() == 0x00000000);
	REQUIRE(VM.readHI() == 0x00000001);
	REQUIRE(VM.readReg(24) == 0x00000000);
	REQUIRE(VM.readReg(25) == 0x00000001);

	VM.simulation();
	VM.simulation();
	VM.simulation();
	VM.simulation();
	VM.simulation();
	REQUIRE(VM.readPC() == 0x00000014);
	REQUIRE(VM.readReg(8) == 0x40000000);
	REQUIRE(VM.readReg(9) == 0xfffffffc);
	REQUIRE(VM.readLO() == 0x00000000);
	REQUIRE(VM.readHI() == 0xffffffff);
	REQUIRE(VM.readReg(24) == 0x00000000);
	REQUIRE(VM.readReg(25) == 0xffffffff);
}

TEST_CASE("vm10") {

	std::string file = R"(
        .data
VALUE = 4
var1:   .word 2
var2:   .word 1073741824 # = 2^30
        .text
main:
        lw $t0, var1
        li $t1, VALUE
        mult $t0, $t1 # 2*4 = 8
        mflo $t8
        mfhi $t9

        lw $t0, var2
        li $t1, VALUE
        mult $t0, $t1 # 1073741824*4 = 4294967296 (overflow)
        mflo $t8
        mfhi $t9

		lw $t0, var1
        li $t1, VALUE
        multu $t0, $t1 # 2*4 = 8
        mflo  $t8
        mfhi  $t9
	
)";
	std::stringstream ss(file);
	TokenList tl = tokenize(ss);
	Parse syntax;
	syntax.parse(tl);
	VirtualMachine VM = syntax.getVM();
	REQUIRE(VM.readPC() == 0x00000000);

	VM.simulation();
	VM.simulation();
	VM.simulation();
	VM.simulation();
	VM.simulation();
	REQUIRE(VM.readPC() == 0x00000005);
	REQUIRE(VM.readReg(8) == 0x00000002);
	REQUIRE(VM.readReg(9) == 0x00000004);
	REQUIRE(VM.readLO() == 0x00000008);
	REQUIRE(VM.readHI() == 0x00000000);
	REQUIRE(VM.readReg(24) == 0x00000008);
	REQUIRE(VM.readReg(25) == 0x00000000);

	VM.simulation();
	VM.simulation();
	VM.simulation();
	VM.simulation();
	VM.simulation();
	REQUIRE(VM.readPC() == 0x0000000a);
	REQUIRE(VM.readReg(8) == 0x40000000);
	REQUIRE(VM.readReg(9) == 0x00000004);
	REQUIRE(VM.readLO() == 0x00000000);
	REQUIRE(VM.readHI() == 0x00000001);
	REQUIRE(VM.readReg(24) == 0x00000000);
	REQUIRE(VM.readReg(25) == 0x00000001);

	VM.simulation();
	VM.simulation();
	VM.simulation();
	VM.simulation();
	VM.simulation();
	REQUIRE(VM.readPC() == 0x0000000f);
	REQUIRE(VM.readReg(8) == 0x00000002);
	REQUIRE(VM.readReg(9) == 0x00000004);
	REQUIRE(VM.readLO() == 0x00000008);
	REQUIRE(VM.readHI() == 0x00000000);
	REQUIRE(VM.readReg(24) == 0x00000008);
	REQUIRE(VM.readReg(25) == 0x00000000);
}


TEST_CASE("vm11") {

	std::string file = R"(
        .data
VALUE = 4
VALUE2 = -4
var1:   .word 2
var2:   .word -2
var3:   .word 1073741824 # = 2^30
        .text
main:
        lw $t0, var1
        li $t1, VALUE
        div $t0, $t1 # 2/4 = 0 rem 2
        mflo $t8
        mfhi $t9

        lw $t0, var2
        lw $t1, var1
        div $t0, $t1 # -2/2 = -1 rem 0 
        mflo $t8
        mfhi $t9

        lw $t0, var3
        li $t1, VALUE
        div $t0, $t1 # 1073741824/4 = 268435456 rem 0
        mflo $t8
        mfhi $t9

        lw $t0, var3
        li $t1, VALUE2
        div $t0, $t1 # 1073741824/-4 = -268435456 rem 0 
        mflo $t8
        mfhi $t9

        # divide by 0
        div $t0, $0
	)";
	std::stringstream ss(file);
	TokenList tl = tokenize(ss);
	Parse syntax;
	syntax.parse(tl);
	VirtualMachine VM = syntax.getVM();
	REQUIRE(VM.readPC() == 0x00000000);

	VM.simulation();
	VM.simulation();
	VM.simulation();
	VM.simulation();
	VM.simulation();
	REQUIRE(VM.readPC() == 0x00000005);
	REQUIRE(VM.readReg(8) == 0x00000002);
	REQUIRE(VM.readReg(9) == 0x00000004);
	REQUIRE(VM.readLO() == 0x00000000);
	REQUIRE(VM.readHI() == 0x00000002);
	REQUIRE(VM.readReg(24) == 0x00000000);
	REQUIRE(VM.readReg(25) == 0x00000002);

	VM.simulation();
	VM.simulation();
	VM.simulation();
	VM.simulation();
	VM.simulation();
	REQUIRE(VM.readPC() == 0x0000000a);
	REQUIRE(VM.readReg(8) == 0xfffffffe);
	REQUIRE(VM.readReg(9) == 0x00000002);
	REQUIRE(VM.readLO() == 0xffffffff);
	REQUIRE(VM.readHI() == 0x00000000);
	REQUIRE(VM.readReg(24) == 0xffffffff);
	REQUIRE(VM.readReg(25) == 0x00000000);

	VM.simulation();
	VM.simulation();
	VM.simulation();
	VM.simulation();
	VM.simulation();
	REQUIRE(VM.readPC() == 0x0000000f);
	REQUIRE(VM.readReg(8) == 0x40000000);
	REQUIRE(VM.readReg(9) == 0x00000004);
	REQUIRE(VM.readLO() == 0x10000000);
	REQUIRE(VM.readHI() == 0x00000000);
	REQUIRE(VM.readReg(24) == 0x10000000);
	REQUIRE(VM.readReg(25) == 0x00000000);

	VM.simulation();
	VM.simulation();
	VM.simulation();
	VM.simulation();
	VM.simulation();
	REQUIRE(VM.readPC() == 0x00000014);
	REQUIRE(VM.readReg(8) == 0x40000000);
	REQUIRE(VM.readReg(9) == 0xfffffffc);
	REQUIRE(VM.readLO() == 0xf0000000);
	REQUIRE(VM.readHI() == 0x00000000);
	REQUIRE(VM.readReg(24) == 0xf0000000);
	REQUIRE(VM.readReg(25) == 0x00000000);
}

TEST_CASE("vm12") {

	std::string file = R"(
        .data
VALUE = 4
var1:   .word 2
var2:   .word 1073741825 # = 2^30+1
        .text
main:
        lw $t0, var1
        li $t1, VALUE
        div $t0, $t1 # 2/4 = 0 rem 2
        mflo $t8
        mfhi $t9

        lw $t0, var2
        li $t1, VALUE
        div $t0, $t1 # 1073741825/4 = 268435456 rem 1
        mflo $t8
        mfhi $t9

        # divide by 0
        div $t0, $0
	
	)";
	std::stringstream ss(file);
	TokenList tl = tokenize(ss);
	Parse syntax;
	syntax.parse(tl);
	VirtualMachine VM = syntax.getVM();
	REQUIRE(VM.readPC() == 0x00000000);

	VM.simulation();
	VM.simulation();
	VM.simulation();
	VM.simulation();
	VM.simulation();
	REQUIRE(VM.readPC() == 0x00000005);
	REQUIRE(VM.readReg(8) == 0x00000002);
	REQUIRE(VM.readReg(9) == 0x00000004);
	REQUIRE(VM.readLO() == 0x00000000);
	REQUIRE(VM.readHI() == 0x00000002);
	REQUIRE(VM.readReg(24) == 0x00000000);
	REQUIRE(VM.readReg(25) == 0x00000002);

	VM.simulation();
	VM.simulation();
	VM.simulation();
	VM.simulation();
	VM.simulation();
	REQUIRE(VM.readPC() == 0x0000000a);
	REQUIRE(VM.readReg(8) == 0x40000001);
	REQUIRE(VM.readReg(9) == 0x00000004);
	REQUIRE(VM.readLO() == 0x10000000);
	REQUIRE(VM.readHI() == 0x00000001);
	REQUIRE(VM.readReg(24) == 0x10000000);
	REQUIRE(VM.readReg(25) == 0x00000001);

	VM.simulation();
	REQUIRE(VM.getStatus() == VM_Status::Simulating);
}

TEST_CASE("vm13") {

	std::string file = R"(
        .data
VALUE = 3
var1:   .word 12
var2:   .word 10
        .text
main:
        lw $t0, var1
        lw $t1, var2
        and $t2, $t0, $t1  
        and $t2, $t0, VALUE
	)";
	std::stringstream ss(file);
	TokenList tl = tokenize(ss);
	Parse syntax;
	syntax.parse(tl);
	VirtualMachine VM = syntax.getVM();
	REQUIRE(VM.readPC() == 0x00000000);

	VM.simulation();
	VM.simulation();
	VM.simulation();

	REQUIRE(VM.readReg(10) == 0x00000008);
	VM.simulation();
	REQUIRE(VM.readReg(10) == 0x00000000);

}

TEST_CASE("vm14") {

	std::string file = R"(
        .data
VALUE = 3
var1:   .word 12
var2:   .word 10
        .text
main:
        lw $t0, var1
        lw $t1, var2
        nor $t2, $t0, $t1  
        nor $t2, $t0, VALUE
	)";
	std::stringstream ss(file);
	TokenList tl = tokenize(ss);
	Parse syntax;
	syntax.parse(tl);
	VirtualMachine VM = syntax.getVM();
	REQUIRE(VM.readPC() == 0x00000000);

	VM.simulation();
	VM.simulation();
	VM.simulation();

	REQUIRE(VM.readReg(10) == 0xfffffff1);
	VM.simulation();
	REQUIRE(VM.readReg(10) == 0xfffffff0);

}

TEST_CASE("vm15") {

	std::string file = R"(
        .data
VALUE = 3
var1:   .word 12
var2:   .word 10
        .text
main:
        lw $t0, var1
        lw $t1, var2
        or $t2, $t0, $t1  
        or $t2, $t0, VALUE
	)";
	std::stringstream ss(file);
	TokenList tl = tokenize(ss);
	Parse syntax;
	syntax.parse(tl);
	VirtualMachine VM = syntax.getVM();
	REQUIRE(VM.readPC() == 0x00000000);

	VM.simulation();
	VM.simulation();
	VM.simulation();

	REQUIRE(VM.readReg(10) == 0x0000000e);
	VM.simulation();
	REQUIRE(VM.readReg(10) == 0x0000000f);

}

TEST_CASE("vm16") {

	std::string file = R"(
        .data
VALUE = 3
var1:   .word 12
var2:   .word 10
        .text
main:
        lw $t0, var1
        lw $t1, var2
        xor $t2, $t0, $t1  
        xor $t2, $t0, VALUE
	)";
	std::stringstream ss(file);
	TokenList tl = tokenize(ss);
	Parse syntax;
	syntax.parse(tl);
	VirtualMachine VM = syntax.getVM();
	REQUIRE(VM.readPC() == 0x00000000);

	VM.simulation();
	VM.simulation();
	VM.simulation();

	REQUIRE(VM.readReg(10) == 0x00000006);
	VM.simulation();
	REQUIRE(VM.readReg(10) == 0x0000000f);

}

TEST_CASE("vm17") {

	std::string file = R"(
        .data
VALUE = 3
var1:   .word 12
var2:   .word 10
        .text
main:
        lw $t0, var1
        lw $t1, var2
        not $t2, $t0
        not $t2, $t1
        not $t2, VALUE
	)";
	std::stringstream ss(file);
	TokenList tl = tokenize(ss);
	Parse syntax;
	syntax.parse(tl);
	VirtualMachine VM = syntax.getVM();
	REQUIRE(VM.readPC() == 0x00000000);

	VM.simulation();
	VM.simulation();
	VM.simulation();

	REQUIRE(VM.readReg(10) == 0xfffffff3);
	VM.simulation();
	REQUIRE(VM.readReg(10) == 0xfffffff5);
	VM.simulation();
	REQUIRE(VM.readReg(10) == 0xfffffffc);
}

TEST_CASE("vm18") {

	std::string file = R"(
        .data
        .text
main:
        nop
        j next
        nop
next:
        nop
        j main
	)";
	std::stringstream ss(file);
	TokenList tl = tokenize(ss);
	Parse syntax;
	syntax.parse(tl);
	VirtualMachine VM = syntax.getVM();
	REQUIRE(VM.readPC() == 0x00000000);

	VM.simulation();
	REQUIRE(VM.readPC() == 0x00000001);
	VM.simulation();
	REQUIRE(VM.readPC() == 0x00000003);
	VM.simulation();
	REQUIRE(VM.readPC() == 0x00000004);
	VM.simulation();
	REQUIRE(VM.readPC() == 0x00000000);
}

TEST_CASE("vm19") {

	std::string file = R"(
        .data
var0:   .word 0
var1:   .word 1
var2:   .word 2
var3:   .word 3
        .text
main:
        lw $t0, var0
        lw $t1, var1
        lw $t2, var2
        lw $t3, var3

        beq $t0, $t1, check1
        beq $t0, $t0, check1
        nop
check1:
        bne $t0, $t0, check2
        bne $t0, $t1, check2
        nop
check2:
        bgt $t0, $t0, check3
        bgt $t3, $t1, check3
        nop
check3:
        bge $t0, $t1, check4
        bge $t3, $t2, check4
        nop
check4:
        blt $t3, $t1, check5
        blt $t1, $t3, check5
        nop
check5:
        ble $t3, $t1, check6
        ble $t3, $t3, check6
        nop
check6:
        nop
        j check6
	)";
	std::stringstream ss(file);
	TokenList tl = tokenize(ss);
	Parse syntax;
	syntax.parse(tl);
	VirtualMachine VM = syntax.getVM();
	REQUIRE(VM.readPC() == 0x00000000);

	VM.simulation();
	VM.simulation();
	VM.simulation();
	VM.simulation();
	REQUIRE(VM.readPC() == 0x00000004);
	VM.simulation();
	REQUIRE(VM.readPC() == 0x00000005);
	VM.simulation();
	REQUIRE(VM.readPC() == 0x00000007);
	VM.simulation();
	REQUIRE(VM.readPC() == 0x00000008);
	VM.simulation();
	REQUIRE(VM.readPC() == 0x0000000a);
	VM.simulation();
	REQUIRE(VM.readPC() == 0x0000000b);
	VM.simulation();
	REQUIRE(VM.readPC() == 0x0000000d);
	VM.simulation();
	REQUIRE(VM.readPC() == 0x0000000e);
	VM.simulation();
	REQUIRE(VM.readPC() == 0x00000010);
	VM.simulation();
	REQUIRE(VM.readPC() == 0x00000011);
	VM.simulation();
	REQUIRE(VM.readPC() == 0x00000013);
	VM.simulation();
	REQUIRE(VM.readPC() == 0x00000014);
	VM.simulation();
	REQUIRE(VM.readPC() == 0x00000016);
	VM.simulation();
	VM.simulation();
	REQUIRE(VM.readPC() == 0x00000016);
}

TEST_CASE("vm20") {

	std::string file = R"(
	# Example program to compute the sum of squares from Jorgensen [2016]

	#---------------------------------------------------------------
	# data declarations
	
	.data
n:		.word 10
sumOfSquares:	.word 0

	#---------------------------------------------------------------
	# the program
	.text
main:
	lw $t0,n
	li $t1,1
	li $t2,0

sumLoop:
	mult $t1, $t1
        mflo $t3
	add $t2, $t2, $t3
	add $t1, $t1, 1
	ble $t1, $t0, sumLoop
	sw  $t2, sumOfSquares

end:
	j end
	)";
	std::stringstream ss(file);
	TokenList tl = tokenize(ss);
	Parse syntax;
	syntax.parse(tl);
	VirtualMachine VM = syntax.getVM();
	REQUIRE(VM.readPC() == 0x00000000);

	for (int i = 0; i < 54; i++) {
		VM.simulation();
	}

	REQUIRE(VM.readMEM(4, 1) == 0x81);
	REQUIRE(VM.readMEM(5, 1) == 0x01);
	REQUIRE(VM.readMEM(6, 1) == 0x00);
	REQUIRE(VM.readMEM(7, 1) == 0x00);
}

TEST_CASE("self test") {
	{
		std::string file = R"(
        .data
VALUE = 4
VALUE2 = -4
var1:   .word 2
var2:   .word -2
var3:   .word 1073741824 # = 2^30
        .text
main:
        lw $t0, var1
        li $t1, VALUE
        divu $t0, $t1 # 2/4 = 0 rem 2
        mflo $t8
        mfhi $t9

		rem $t0, $t1, $t2
	)";
		std::stringstream ss(file);
		TokenList tl = tokenize(ss);
		Parse syntax;
		syntax.parse(tl);
		VirtualMachine VM = syntax.getVM();
		REQUIRE(VM.readPC() == 0x00000000);

		VM.simulation();
		VM.simulation();
		VM.simulation();
		VM.simulation();
		VM.simulation();
		REQUIRE(VM.readPC() == 0x00000005);
		REQUIRE(VM.readReg(8) == 0x00000002);
		REQUIRE(VM.readReg(9) == 0x00000004);
		REQUIRE(VM.readLO() == 0x00000000);
		REQUIRE(VM.readHI() == 0x00000002);
		REQUIRE(VM.readReg(24) == 0x00000000);
		REQUIRE(VM.readReg(25) == 0x00000002);
	}

}