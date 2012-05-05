//REMOVED CODE

//COIN FLIP CODE

for ( i = 0; i < tt_per_rank; i++ ) {
		for ( j = 0; j < tt_total; j++ ) {
			
			/* ==================== COIN FLIP ===================
			To decide which territory's rank computes the result on an edge:

			If the flips are the same, the territory with the lower ID 
			  is responsible for the calculation.
			If the flips are different, the terrirory with the higher ID
			  is responsible for the calculation.
			This should evenly distribute the work among the ranks and should
			  solve the lower-IDed-territories-have-more-work problem. 
			There might be a more efficient way to to this, 
			  but this seemed pretty good when I was working on it.  -DJS    */

			myBattleFlips[i][j] = diceRoll(COIN_FLIP);
			//printf("[%d] MyBattleFlips[%d][%d]: %d\n", myRank, i, j, myBattleFlips[i][j]);
		}
	}


//MPI HOT POTATO

	for ( j = 0; j < tt_per_rank; j++ ) {
			for ( k = 0; k < tt_total; k++ ) {
				printf("[%d] (T %d -- T %d ) RecvBufAdjSlice[%d][%d] = %d\n", myRank, j+recvBuffer->row_offset, k, j, k, ACC(recvBuffer->adjSlice, j, k));
			}
		}

		for ( j = 0; j < tt_per_rank; j++ ) { // J -> my territories
			for ( k = 0; k < tt_per_rank; k++ ) { // K -> territories in currently received slice

				//Don't even bother if we're on the diagonal
				if ( tt_offset+j == recvBuffer->row_offset+k ) { continue; }

				//printf("[%d] RecvBattleFlips[%d][%d]: %d\n", myRank, i, j, recvBuffer->battleFlip[k][j]);

				printf("[%d] I [terr %d] flipped %d; Foe [terr %d] (rank %d) flipped %d\n", myRank, tt_offset+j, myBattleFlips[ j ][ recvBuffer->row_offset+k ], recvBuffer->row_offset+k, recvBuffer->source_rank, ACC(recvBuffer->battleFlip, k, tt_offset+j) );
				char sameFlip = ACC(recvBuffer->battleFlip, k, tt_offset+j) == myBattleFlips[j][recvBuffer->row_offset+k];
				char iAmHigher = tt_offset+j > recvBuffer->row_offset+k;
				char isMyJob;
				if (sameFlip == 1) {
					if (iAmHigher == 0) {
						isMyJob = 1;
					} else {
						isMyJob = 0;
					}
				} else { //Different Flips
					if (iAmHigher == 1) {
						isMyJob = 1;
					} else {
						isMyJob = 0;
					}
				}
				printf("[%d] J: %d; J-Offset: %d; K: %d\n", myRank, j, tt_offset, k);
				printf( "[%d] My Job: %d; Borders: %d\n", myRank, isMyJob, ACC(recvBuffer->adjSlice, k, tt_offset+j) );

				//If there's an adjacency, I'll decide the battle if it's my job to do so
				if ( ACC(recvBuffer->adjSlice, k, tt_offset+j) == 1 && isMyJob == 1 ) {
					
					int myNode_power = edgeActivity[ j ][ recvBuffer->row_offset+k ];
					int otherNode_power = ACC(recvBuffer->edgeSlice, k, tt_offset+j);

					int result = do_battle( myNode_power, otherNode_power );

					//IF RESULT < 0:
					// otherNode won (with *-result* troops surviving the attack)
					// but otherNode will see this as a win, so we'll store
					// it as a positive value (negating it)
					//IF RESULT > 0:
					// myNode won (with *result* troops surviving the attack)
					// but this is the otherNode's slot, so we'll negate it
					ACC(recvBuffer->edgeResult, k, tt_offset+j) = -1 * result;
					printf("[%d] Battle between MyNode %d and OtherNode %d: %d\n", myRank, tt_offset+j, recvBuffer->row_offset+k, result);					
				}
			}
		}

		printf("[%d] Boxing up rank %d's data...\n", myRank, recvBuffer->source_rank);
		//Information we've just finished working on is in recvBuffer.
		//Move the received info to the send buffer.
		sendBuffer->source_rank = recvBuffer->source_rank;
		sendBuffer->row_offset = recvBuffer->row_offset;

		for ( j = 0; j < tt_per_rank; j++ ) {
			memcpy( sendBuffer->adjSlice, recvBuffer->adjSlice, tt_total * tt_per_rank * sizeof(int) );
			memcpy( sendBuffer->edgeSlice, recvBuffer->edgeSlice, tt_total * tt_per_rank * sizeof(int) );
			memcpy( sendBuffer->battleFlip, recvBuffer->battleFlip, tt_total * tt_per_rank * sizeof(int) );
			memcpy( sendBuffer->edgeResult, recvBuffer->edgeResult, tt_total * tt_per_rank * sizeof(int) );
		}
		MPI_Irecv( recvBuffer, 1, MPI_BCMSG, PREV_RANK_FROM(myRank), 0, MPI_COMM_WORLD, &requests[RECV] );
		MPI_Isend( sendBuffer, 1, MPI_BCMSG, NEXT_RANK_FROM(myRank), 0, MPI_COMM_WORLD, &requests[SEND] );
		MPI_Waitall( 2, requests, statuses );
		for ( j = 0; j < 2; j++ ) {
			printf("[%d] J=%d, Error = %d\n", myRank, j, statuses[j].MPI_ERROR);
		}

		printf("[%d] Completed Irecv of data from rank %d\n", myRank, recvBuffer->source_rank);


		for ( j = 0; j < tt_per_rank; j++ ) {
			for ( k = 0; k < tt_total; k++ ) {
				printf("[%d] (T %d -- T %d ) RecvBufAdjSlice[%d][%d] = %d\n", myRank, j+recvBuffer->row_offset, k, j, k, ACC(recvBuffer->adjSlice, j, k));
			}
		}