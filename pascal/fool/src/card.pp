unit card; { card.pp }
{
    contains data structures and basic methods for cards
}
interface
const
    NumberOfSuits = 4;
    NumberOfValues = 9;
type
    CardSuit = (Clubs, Diamonds, Hearts, Spades);
    CardValue = (Six, Seven, Eight, Nine, Ten, Jack, Queen, King, Ace);
    CardPtr = ^PlayingCard;
    PlayingCard = record
        suit: CardSuit;
        value: CardValue;
    end;
var
    AllSuits: array [1..NumberOfSuits] of CardSuit = 
        (Clubs, Diamonds, Hearts, Spades);
    AllValues: array [1..NumberOfValues] of CardValue = 
        (Six, Seven, Eight, Nine, Ten, Jack, Queen, King, Ace);

function CompareCards(card1, card2: CardPtr; TrumpSuit: CardSuit): integer;
function RandomCard: PlayingCard;


implementation

function CompareCards(card1, card2: CardPtr; TrumpSuit: CardSuit): integer;
begin
    if (card1^.suit = TrumpSuit) and (card2^.suit <> TrumpSuit) then
        CompareCards := -1
    else if (card1^.suit <> TrumpSuit) and (card2^.suit = TrumpSuit) then
        CompareCards := 1
    else if (card1^.suit <> card2^.suit) or (card1^.value = card2^.value) then
        CompareCards := 0
    else if card1^.value > card2^.value then
        CompareCards := -1
    else
        CompareCards := 1
end;

function RandomCard: PlayingCard;
begin
    RandomCard.suit := AllSuits[random(NumberOfSuits) + 1];
    RandomCard.value := AllValues[random(NumberOfValues) + 1]
end;

end.
