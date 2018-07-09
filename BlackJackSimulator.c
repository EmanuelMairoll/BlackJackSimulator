#include <stdio.h>
#include <stdlib.h>
#include <time.h>

#define START_HANDS 64
#define MAX_HANDS 96

#define PLAYER_MAX_CARDS 22

#define DECISION_STAND 0
#define DECISION_HIT 1
#define DECISION_DOUBLE 2
#define DECISION_SPLIT 3
#define DECISION_SURRENDER 4

#define STATE_NORMAL 0
#define STATE_SPLIT_HAND 1
#define STATE_DOUBLE 2
#define STATE_SURRENDERED 3

#define DEALER_SCORE_MIN 17
#define DEALER_NO_PEEK 1
#define DECKS 6

#define ROUNDS_TO_SIMULATE 1000000
#define LOGMODE 0

void playRound(unsigned long* playedHands, long* balance);
char cardSum(char cards[], char cardCount, char* isSoft);
char isBlackJack(char cards[], char cardCount);
char decide(char dealerCard, char cards[], char cardCount, char canSurrender);
void resetDeck(void);
char drawCard(void);

char deck[52 * DECKS];
int nextDrawIndex = 0;

int main(int argc, char *argv[]) {

	time_t start, stop;
	time(&start);
	srand(time(NULL));

	printf("==========================================BLACKJACK STRATEGY TESTER==========================================\n");
	unsigned long playedHands = 0;
	long balance = 0;
		
	unsigned long simulatedRounds;
	for(simulatedRounds = 1; simulatedRounds <= ROUNDS_TO_SIMULATE; simulatedRounds++){
		playRound(&playedHands, &balance);
				
		if (simulatedRounds % (ROUNDS_TO_SIMULATE / 20) == 0){
			printf("|| Simulated Rounds: %-12luSimulated Hands: %-15luAverage Profit percentage: %f%%\n", simulatedRounds, playedHands, (balance / (float)playedHands));
		}
	}
	
	time(&stop);
	
	printf("=============================================================================================================\n");
	printf("|| Simulated %lu Rounds with %i Players each, finishing in %i Seconds\n", simulatedRounds - 1, START_HANDS, (int)difftime(stop, start));
	printf("|| Overall Profit Percentage for this Strategy: %f%%\n", (balance / (float)playedHands));
}

void playRound(unsigned long* playedHands, long* balance){
	//////////////////////////////DRAWING CARDS//////////////////////////////
	resetDeck();
	
	char dealerHand[PLAYER_MAX_CARDS];
	dealerHand[0] = drawCard();
	dealerHand[1] = drawCard();
	char nextFreeIndexInDealerHand = 2;
		
	char playerHands[MAX_HANDS][PLAYER_MAX_CARDS];
	char nextFreeCardIndex[MAX_HANDS];
	char playerState[MAX_HANDS];
	int nextFreeHandIndex = START_HANDS;
	for (int handIndex = 0; handIndex < START_HANDS; handIndex++){
		playerHands[handIndex][0] = drawCard();
		playerHands[handIndex][1] = drawCard();
		nextFreeCardIndex[handIndex] = 2;
		playerState[handIndex] = STATE_NORMAL;
	}
		
	//////////////////////////////DEALING PLAYERS//////////////////////////////
	char _;
	
	if (!isBlackJack(dealerHand, nextFreeIndexInDealerHand || DEALER_NO_PEEK)){
		for (int handIndex = 0; handIndex < nextFreeHandIndex; handIndex++){
			if (isBlackJack(playerHands[handIndex], nextFreeCardIndex[handIndex])){
				goto nextHand;
			}	
				
			while(1){
				char decision = decide(dealerHand[0], playerHands[handIndex], nextFreeCardIndex[handIndex], playerState[handIndex] != STATE_SPLIT_HAND);
				switch(decision){
					case DECISION_STAND:
						goto nextHand;

					case DECISION_HIT:
						playerHands[handIndex][nextFreeCardIndex[handIndex]] = drawCard();
						nextFreeCardIndex[handIndex]++;
						break;

					case DECISION_DOUBLE:
						if (nextFreeCardIndex[handIndex]>2){
							goto nextHand;
						}
						playerState[handIndex] = STATE_DOUBLE;
						playerHands[handIndex][nextFreeCardIndex[handIndex]] = drawCard();
						nextFreeCardIndex[handIndex]++;
						goto nextHand;

					case DECISION_SPLIT:
						if (nextFreeHandIndex >= MAX_HANDS || playerHands[handIndex][0] != playerHands[handIndex][1]){
							goto nextHand;
						} else {
							playerState[handIndex] = STATE_SPLIT_HAND;
							
							playerHands[nextFreeHandIndex][0] = playerHands[handIndex][1];
							playerHands[handIndex][1] = drawCard();
							playerHands[nextFreeHandIndex][1] = drawCard();

							nextFreeCardIndex[nextFreeHandIndex] = 2;
							playerState[nextFreeHandIndex] = STATE_SPLIT_HAND;
							nextFreeHandIndex++;
							break;
						}

					case DECISION_SURRENDER:
						playerState[handIndex] = STATE_SURRENDERED;
						goto nextHand;
				}
				
				if (cardSum(playerHands[handIndex], nextFreeCardIndex[handIndex], &_) > 21){
					goto nextHand;
				}
			}
			
			nextHand: continue;						//https://xkcd.com/292/
		}
	}
	//////////////////////////////DEALING DEALER//////////////////////////////
	
	while(cardSum(dealerHand, nextFreeIndexInDealerHand, &_) < DEALER_SCORE_MIN){
		dealerHand[nextFreeIndexInDealerHand] = drawCard();
		nextFreeIndexInDealerHand++;
	}	
		
	//////////////////////////////PAYOUT//////////////////////////////
		
	char dealerSum = cardSum(dealerHand, nextFreeIndexInDealerHand, &_);
	if (LOGMODE) printf("\n\n\nDEALER: ");
	for(int i = 0; i < nextFreeIndexInDealerHand; i++){
		if (LOGMODE) printf("%i ", dealerHand[i]);
	}
	if (LOGMODE) printf("\n");
	*playedHands += 64;
	for (int handIndex = 0; handIndex < nextFreeHandIndex; handIndex++){
		char doubled = playerState[handIndex] == STATE_DOUBLE;	
		*balance -= doubled ? 200 : 100;

		if (LOGMODE) printf("HAND (%i): ", nextFreeCardIndex[handIndex]);
		for(int i = 0; i < nextFreeCardIndex[handIndex]; i++){
			if (LOGMODE) printf("%i ", playerHands[handIndex][i]);
		}
		if (playerState[handIndex] == STATE_SURRENDERED){
			*balance += 1;
			if (LOGMODE) printf("SURRENDERED\n");
			continue;
		}
			
		if (isBlackJack(playerHands[handIndex], nextFreeCardIndex[handIndex])){
			//CAN NOT BE DOUBLE STATE
			if (isBlackJack(dealerHand, nextFreeIndexInDealerHand)){
				*balance += 100;		
				if (LOGMODE) printf("TIE (BLACKJACK)\n");
			}else{
				*balance += 250;
				if (LOGMODE) printf("WIN (BLACKJACK)\n");
			}
			continue;
		} else {
			if (isBlackJack(dealerHand, nextFreeIndexInDealerHand)){
				if (LOGMODE) printf("LOSE (BLACKJACK)\n");
				continue;
			}
		}
		
		char handSum = cardSum(playerHands[handIndex], nextFreeCardIndex[handIndex], &_);
		if (LOGMODE) printf(" SUM: %i      ", handSum);
		if (doubled && LOGMODE) printf("DOUBLED ");
		if (handSum > 21 || (handSum < dealerSum && dealerSum <= 21)) {
			if (LOGMODE) printf("LOSE\n");
		} else if (dealerSum > 21 || handSum > dealerSum) {
			*balance += doubled ? 400 : 200;
			if (LOGMODE) printf("WIN\n");
		} else {
			*balance += doubled ? 200 : 100;
			if (LOGMODE) printf("TIE\n");
		}
		continue;
	}	
}

char cardSum(char cards[], char cardCount, char* isSoft){
	char sum = 0;
	char hasAce = 0;

	for (int i = 0; i < cardCount; i++){
		sum += cards[i];
		hasAce |= cards[i] == 11;
	}
	
	if (hasAce){
		if (sum > 21){
			*isSoft = 0;
			sum -= 10;	
		} else {
			*isSoft = 1;
		}
	}
	return sum;
	
}

char isBlackJack(char cards[], char cardCount){
	return ((cards[0] == 11 && cards[1] == 10) || (cards[0] == 10 && cards[1] == 11)) && cardCount == 2;
}

void resetDeck(void){
	for(int deckIndex = 0; deckIndex < DECKS * 4; deckIndex++){
		for (char cardIndex = 0; cardIndex < 13; cardIndex++){
			int index = deckIndex * 13 + cardIndex;
			char pip = (cardIndex + 2 >= 12) ? 10 : cardIndex + 2;
			deck[index] = pip;
		}
	}
		
	int deckSize = sizeof(deck);
	for (int i = deckSize - 1; i > 0; i--)
	{
		int j = rand() % (i + 1);
		
		char temp = deck[j];
		deck[j] = deck[i];
		deck[i] = temp;
	}
	
	nextDrawIndex = 0;
}

char drawCard(void){
	if (nextDrawIndex >= sizeof(deck)){
		resetDeck();
	}
	
	return deck[nextDrawIndex++];
}


char decide(char dealerCard, char cards[], char cardCount, char canSurrender){
	char isSoft = 0;
	char sum = cardSum(cards, cardCount, &isSoft);

	if (cardCount == 2 && cards[0] == cards[1]){
		if (cards[0] == 11 || cards[0] == 8){
			return DECISION_SPLIT;
		} else if ((cards[0] == 2 || cards[0] == 3 || cards[0] == 7) && (dealerCard < 8)){
			return DECISION_SPLIT;
		} else if (cards[0] == 6 && (dealerCard < 7)){
			return DECISION_SPLIT;
		}
	}

	if (cardCount == 2 && (sum == 11 || (sum == 10 && dealerCard < 10))){
		return DECISION_DOUBLE;
	}
	
	if (isSoft && sum < 18){
		return DECISION_HIT;
	}
	
	if (sum < ((dealerCard < 7) ? 12 : 17)){
		return DECISION_HIT;
	} else {
		return DECISION_STAND;
	}	
}


//TODO: PLAYED ROUND AT SPLIT?