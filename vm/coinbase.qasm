#
# registers
# -> common
#	r0 - r15
#	r0 - script result & many other cases
# -> signature (protected)
#	s0 - s15
#	s15 - sich check result
# -> condition check
#	c0 - c15
#	all check... functions put result into c0 register
# -> "this" register
#	th0 - transaction pointer (hash), initialized before program run by context "this" transaction

# ----------------------------
# tx type: coinbase
# ----------------------------

#
# input[0] program
	mov		s15, 0x01
	mov		r1, height
	mov		r0, 0x01
	ret

#
# output[0] program
	mov		r0, 0x2023947283749211
	pushd		r0					# push destination address and [asset/txid/index]
	cmpe		r0, s0					# suppose, that s0 contains input pubkey, result -> c0
	mov		r0, c0					# set result to r0
