unit hand; { hand.pp }
{
    Contains data structure and functionality for a player's hand
}
interface
uses card, deck;
const
    NormalHandSize = 6;
type
    PlayerHandPtr = ^PlayerHand;
    PlayerHand = array [1..DeckSize] of CardPtr;

procedure InitHand(var h: PlayerHand);
function NumCardsInHand(var h: PlayerHand): integer;
procedure AddCard(var h: PlayerHand; c: CardPtr; var success: boolean);
procedure RemoveCard(var h: PlayerHand; c: CardPtr; var success: boolean);


implementation

procedure InitHand(var h: PlayerHand);
var
    i: integer;
begin
    for i := 1 to DeckSize do
        h[i] := nil
end;

function NumCardsInHand(var h: PlayerHand): integer;
var
    i: integer;
begin
    NumCardsInHand := 0;
    for i := 1 to DeckSize do
        if h[i] <> nil then
            NumCardsInHand := NumCardsInHand + 1
end;

function CardIsInHand(var h: PlayerHand; c: CardPtr): boolean; forward;

procedure AddCard(var h: PlayerHand; c: CardPtr; var success: boolean);
var
    i: integer;
begin
    success := (NumCardsInHand(h) < DeckSize) and (not CardIsInHand(h, c));
    if success then
        for i := 1 to DeckSize do
            if h[i] = nil then
            begin
                h[i] := c;
                break
            end
end;

procedure RemoveCard(var h: PlayerHand; c: CardPtr; var success: boolean);
var
    i: integer;
begin
    success := CardIsInHand(h, c);
    if success then
        for i := 1 to DeckSize do
            if h[i] = c then
            begin
                h[i] := nil;
                break
            end
end;

function CardIsInHand(var h: PlayerHand; c: CardPtr): boolean;
var
    i: integer;
begin
    CardIsInHand := false;
    for i := 1 to DeckSize do
        if h[i] = c then
        begin
            CardIsInHand := true;
            break
        end
end;

end.
