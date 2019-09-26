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
#	th1 - transaction type: coinbase, spend, put_order, refund_order, ...

# ----------------------------
# tx_type: refund_order
# ----------------------------
# limitations:
#	input can be: put_order

#
# input[0] program (executed with previous out - common for market and user previous output)
	mov			s0, 0x2023947283749211				# pubkey - our address
	mov			s1, 0x85389659837439875498347598357	# signature
	lhash256	s2									# hash link, result -> s2
	checksig										# s0 - keym s1 - sig, s2 - message -> result c0
	mov			s15, c0								# store checksig result to the output sig result register s15
	pulld											# pull-out [link.asset, link.hash, link.index]

#
# output[0] program (two possible recipients: market and user-initiator)
	mov			r0, 0x2023947283749211	# users address (order source)
	pushd		r0
	cmpe		r0, s0					# s0 - target address (users, refund)
	jmpt		:0						# jump to offset if c0 == true
	mov			r0, 0x01				# reset r0: all fine, just check
	ret									# return: [s15 = checksig result, r0 = 0x01: if s15 == 0x01 && r0 == 0x01 => successed] 
0:	cmpe		th1, 0x48				# put order
	jmpt		:1
	mov			r0, 0x00
	ret
1:	mov			r0, 0x01
	ret
