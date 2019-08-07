#
# registers
# -> common
#	r0 - r31
#	r0 - script result & many other cases
# -> signature (protected)
#	s0 - s7
#	s2 - read-only
#	s7 - sign check result, read-only
#	s0 - set if _empty_, otherwise - exception
# -> condition check
#	c0 - read-only, all cmp... functions put result into c0 register
# -> "this" registers, constant registers (initialized before program run by context "this" transaction)
#	th0 - th15
#	th0 - transaction hash
#	th1 - transaction type
#	th2 - transaction input number
#	th3 - transaction output number
# -> amount
#	a0 - a7
#	a0 - set, if a0 was _empty_
#	a1 - set, if a1 was _empty_
#	a7 - amount check result, read-only
# -> destination register
#	d0 - set if was _empty_
# -> utxo pull result, read-only
#	p0 - set by dtxo 0 - if corresponding utxo is absent (already spent/linked or not found yet), 1 - otherwise
# -> address check result
#	e0 - read-only, address equality result (eqaddr) 

# ----------------------------
# tx type: asset_type
# ----------------------------

#
# input[0] program
	mov		r0, 0x01

#
# output[0] program (limited)
	mov			d0, 0x2023947283749211				# destination address
	mov			a0, 0x0000000000010000				# supply, uint64
	mov			a1, 0x3459385793464634				# pedersen commitment
	mov			a2, 0x4535345345345345				# blinding key
	checka											# verify amount [a0, a1, a2 => result a7]
	atxoa											# push for d0 public amount - [d0, a0, a7]
	eqaddr											# suppose, that s0 contains input pubkey, result -> c1
	pat												# push asset type
	cmpe		th1, 0x0021							# only tx emission can be
	mov			r0, c0
	ret

#
# result: [r0, e0, s7 (later)]

#
# output[0] program (pegged)
	mov			d0, 0x2023947283749211				# destination address
	atxo											# push for d0 public amount - [d0, a0, a7]
	eqaddr											# suppose, that s0 contains input pubkey, result -> c1
	pat												# push asset type
	cmpe		th1, 0x0021							# only tx emission can be
	mov			r0, c0
	ret
#
# result: [r0, e0, s7 (later)]
