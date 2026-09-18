# Sum 1^2 + ... + 10^2 = 385; result bytes at addresses 4..7.
.data
n: .word 10
result: .word 0
.text
main:
    lw $t0, n
    li $t1, 1
    li $t2, 0
loop:
    mult $t1, $t1
    mflo $t3
    addu $t2, $t2, $t3
    addu $t1, $t1, 1
    ble $t1, $t0, loop
    sw $t2, result
end:
    j end
