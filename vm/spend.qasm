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
#	all check[X] functions put result into c0 register
# -> "this" registers
#	th0 - transaction pointer (hash), initialized before program run by context "this" transaction
# -> tx register
#	txid - current tx id

# ----------------------------
# tx type: spend
# ----------------------------

#
# input[0] program
	mov		s0, 0x2023947283749211			# pubkey - our address
        mov		s1, 0x85389659837439875498347598357	# signature
	lhash256	s2					# hash link, result -> s2
	checksig						# s0 - keym s1 - sig, s2 - message -> result c0
	mov		s15, c0
	pulld							# pull-out destination [link.asset, link.hash, link.index] from "unlinked\unspend"

#
# output[0] program
	mov		r0, 0x2023947283749211			# set destination address
	pushd		r0					# push destination address and [asset, txid, index] (macro command - put this out to the "unlinked\unspend" list) 
	cmpe		r0, s0					# suppose, that s0 contains input pubkey, result -> c0
	mov		r0, c0					# move c0 to r0
