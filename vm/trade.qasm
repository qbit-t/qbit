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
# -> "this" register, constant registers (initialized before program run by context "this" transaction)
#	th0 - transaction hash
#	th1 - transaction type

# ----------------------------
# tx_type: trade
# ----------------------------
# limitations:
#	input can be: put_order

#
# input[0] program (input of bid order)
	mov				s0, 0x8276423768264223				# pubkey - market addres
	mov				s1, 0x85389659837439875498347598357	# signature
	lhash256		s2									# hash link, result -> s2
	checksig											# s0 - keym s1 - sig, s2 - message -> result c0
	mov				s15, c0								# store checksig result to the output sig result register s15
	pulld												# pull-out [link.asset, link.hash, link.index]
#
# input[1] program (input of ask order)
	mov				s0, 0x8276423768264223				# pubkey - market addres
	mov				s1, 0x85389659837439875498347598352	# signature
	lhash256		s2									# hash link, result -> s2
	checksiп											# s0 - keym s1 - sig, s2 - message -> result c0
	mov				s15, c0								# store checksig result to the output sig result register s15
	pulld												# pull-out [link.asset, link.hash, link.index]

# --------
# bid side
# --------
# output[0].asset = ask.asset
# output[0].amount = ask.amount
# output[0].conditions (bid receiver of ask order side)
	mov			r0, 0x2524523523452345			# bid address
	pushd		r0
	cmpe		r0, s0							# s0 - target address
	mov			r0, c0

# --------
# ask side
# --------
# output[1].asset = bid.asset
# output[1].amount = bid.amount
# output[1].conditions (ask receiver of bid order side)
	mov			r0, 0x2985735298547987			# ask address   
	pushd		r0
	cmpe		r0, s0							# s0 - target address 
	mov			r0, c0

