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
# tx type: coinbase
# ----------------------------

#
# input[0] program
	mov		r0, 0x01

#
# output[0] program
	mov			d0, 0x2023947283749211	# destination address
	mov			a0, 0x000000000000000a	# mining/verification reward, uint64
	mov			a1, 0x3459385793857938	# pedersen commitment for open tx
	mov			a2, 0x4535345345345345	# blinding key - need to calculate commitment to verify futher thransition to the privacy
	checka								# verify amount [a0, a1, a2 => result a7]
	atxoa								# push for d0 public amount - [d0, a0, a7]
	eqaddr								# suppose, that s0 contains input pubkey, result -> e0
	mov			r0, 0x01
	ret

#
# result: [r0, e0, s7 (later), a7]

