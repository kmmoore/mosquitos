.globl random_read_rdseed
random_read_rdseed:
    rdseed %rax
    jnc random_read_rdseed
    ret
