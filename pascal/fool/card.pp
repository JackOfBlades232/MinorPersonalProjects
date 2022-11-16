unit card; { card.pp }
{
    contains data structures for playing cards
}
interface
type
    CardSuit = (Clubs, Diamonds, Hearts, Spades);
    CardValue = (6, 7, 8, 9, 10, Jack, Queen, King, Ace);
    card = record
        suit: CardSuit;
        value: CardValue;
    end;

function CompareCards(card1, card2: card; TrumpSuit: CardSuit): integer;
{ function RandomCard: card; }

implementation

function CompareCards(card1, card2: card; TrumpSuit: CardSuit): integer;
begin
    if (card1.suit = TrumpSuit) and (card2.suit <> TrumpSuit) then
        CompareCards := -1
    else if (card1.suit <> TrumpSuit) and (card2.suit = TrumpSuit) then
        CompareCards := 1
    else if (card1.suit <> card2.suit) or (card1.value = card2.value) then
        CompareCards := 0
    else if card1.value > card2.value then
        CompareCards := -1
    else
        CompareCards := 1
end;

function RandomCard: card;
