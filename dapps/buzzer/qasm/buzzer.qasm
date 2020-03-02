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

# -----------------------------------------------
# tx type: buzzer (main buzz enity, buzz source)
# -----------------------------------------------

#
# input[0] program
	# qbit-fee
	mov			s0, 0x2023947283749211				# pubkey - our address
	mov			s1, 0x85389659837439875498347598357	# signature
	lhash256	s2									# hash link, result -> s2
	checksig										# s0 - key, s1 - sig, s2 - message -> result s7 [s7-r/o]
	dtxo											# p0 - pop utxo result

#
# input[1] program
	# dapp
	mov			s0, 0x2023947283749211				# pubkey - our address
	mov			s1, 0x85389659837439875498347598357	# signature
	lhash256	s2									# hash link, result -> s2
	checksig										# s0 - key, s1 - sig, s2 - message -> result s7 [s7-r/o]
	dtxo											# p0 - pop utxo result

#
# output[0] program
	# qbit-fee
	mov			d0, 0xFFFA							# miner mark
	mov			a0, 0x0000000000000009				# amount, uint64
	mov			a1, 0x3459385793464634				# pedersen commitment for open tx
	mov			a2, 0x4535345345345345				# blinding key - need to calculate commitment to verify futher 
	checka											# verify amount [a0, a1, a2 => result a7]
	ret

#
# output[1] program
	pen												# push entity (entity name should be provided)
	ptxo											# push unlinked out (entity/out index)
	mov			r0, 0x01
	ret
