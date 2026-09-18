# Run, break, inspect $t0, then step or run again.
.text
main:
    addu $t0, $t0, 1
    j main
