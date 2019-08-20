#
# registers
# -> common
#	r0 - r15
#	r0 - script result & many other cases
# -> signature (protected)
#	s0 - s15
#	s15 - sign check result, read-only
#	s0  - set if _empty_, otherwise - exception
# -> condition check
#	c0 - c15
#	all check... functions put result into c0 register
# -> "this" register, constant registers (initialized before program run by context "this" transaction)
#	th0 - transaction hash
#	th1 - transaction type
#	th2 - transaction input number

# ----------------------------
# tx_type: put_order
# ----------------------------
# limitations:
#	input can be: coinbase, spend, refund_order

#
# input[0] program (executed with previous out - ability of 'spend')
	mov		s0, 0x2023947283749211				# pubkey - our address
	mov		s1, 0x85389659837439875498347598357	# signature
	lhash256	s2								# hash link, result -> s2
	checksig									# s0 - keym s1 - sig, s2 - message -> result c0
	mov		s15, c0								# store checksig result to the output sig result register s15
	pulld										# pull-out [link.asset, link.hash, link.index]
												# it is not necessary for tx types put_order, refund_order
												# because engine and network will check for given input and reject 
												# if it is already USED
#
# output[0] program (two possible recipients: market and user-initiator)
	mov		r0, 0x8276423768264223				# market address
	pushd	r0
	cmpe	r0, s0								# s0 - target address (market)
	jmpt	:0									# jump to label if c0 == true
	mov		r0, 0x2023947283749211				# users address (order source)
	pushd	r0
	cmpe	r0, s0								# s0 - target address (users, refund)
	jmpt	:1									# jump to offset if c0 == true
	mov		r0, 0x01							# reset r0: all fine, just check
	ret											# return: [s15 = checksig result, r0 = 0x01: 
												#	if s15 == 0x01 && r0 == 0x01 => successed] 
0:	cmpe	th1, 0x50							# trade tx
	jmpt	:2
	mov		r0, 0x00
	ret
2:	mov		r0, 0x01
	ret
1:	cmpe	th1, 0x49							# refund\cancel order
	jmpt	:3
	mov		r0, 0x00
	ret
3:	mov		r0, 0x01
	ret

я вот подумал тут на предмет выставления и страйка ордеров... здесь ведь тоже можно применить "расширенную" модель свопа:
- выставление ордера - это, к примеру, создание транзы с нарезанными на лоты подписанными входами для квот юнита (по-сути это смарт-контракт, который предоставляет свободные концы для обработки и может выполнить возврат непотраченных концов - в случае отмены ордера)
- выставление ордера с другой стороны - это транзы с нарезанными и подписанными входами для бэйз юнита
- матчинг (конкретный узел маркетный) это обработка подписанных концов, только обработка будет осуществляться непосредственно энжином на с++, он по-сути будет при приходе ордера вызывать метод смарт-контракта для получения концов с одной стороны и с другой; при этом при вызове метода отмены (это специальная неперсистентная транза), смарт-контракт сформирует (точнее скомпонует) транзакцию возврата: оставшиеся подписанные концы запихнёт во входы, а выходом будет адрес иницииаторы вызова.

минусы:
- транзы могут получаться довольно большими
- возможно уменьшение скорости процессинга на стороне маркетного узла
- многократное увеличение utxo

плюсы:
- схема, в которой отсутствует третья сторона и юзеры торгуют между собой
- не требуется депозит на адрес маркета
- адрес маркета - это всего-лишь атрибут, который позволяет принимать и обрабатывать только "свои" ордеры
- маркетному узлу не нужен свой уникальный приватный ключ

обязательным условием является наличие как минимум 2х узлов, которые будут дублировать действия основного для того, чтобы получить один и тот же результат в рамках маркетного консенсуса (при этом локальный блок будет формироваться чаще, чем работает вся сеть, скажем, 5 секунд, а может и быстрее)
