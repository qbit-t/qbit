
### Instant swap

## Common approach

Network architecture should be as follows: 
- Gatekeepers - one or more for the source DLT (Bitcoin, for example). Implement import/export mechanics. 
- Validators - many. Supports corresponding signing protocol for destination DLT (Bitcoin, for example).

User creates or making import of DLT key pair. Then he/she initiates multisig address creation within corresponding gatekeeper and randomly selected validators set. User creating special transaction - "create multisig address" (Bitcoin, for example) - and broadcast it into qbit network. 

Let's signing scheme will be 5-3 (up to 15). Thus, will be 4 randomly selected Validators, participating in this multisig address creation. Each of 4 Validators will create a new pair and provide pubkey for multisig address through new transaction and broadcast it.

Then, user receives 4 txs, create multisig address and broadcasts it alone with his qbit base address. Gatekeeper will receive new address information for future mapping (to import, for example Bitcoin).

To make coins available to trade user must to send his native coins (Bitcoin, for example) to the newly created multisig address. When native tx was confirmed in source network, corresponding Gatekeeper will catch up this tx and make destination pegged tx in the qbit network to the corresponding user's qbit base address (using internal asset type q-Bitcoin, bound to the Bitcoin, for example).

Then user will be ble to make order-contract to sell or buy something for his/her 'imported' coins.

When his/her order fullfiled, user will be able to make 'export' from qbit asset to the native DLT coins (he/she trade his q-Litecoin for q-Bitcoin). Thus user going to make transaction to the external DLT address (Bitcoin). Hi/she making special internal transaction from his/her qbit address to the external DLT (Bitcoin, for example) address. Corresponding Gatekeeper catch this tx and broadcast tx to ask Validators sign outgoing tx. Validators will provide partial signature only in case when requested to export amount truly owned by the sender (according to senders qbit base address).

