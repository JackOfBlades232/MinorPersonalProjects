unit hand; { hand.pp }
interface
uses card, deck;
{
    Contains data structure and functionality for a player's hand
}
const
    NormalHandSize = 6;
type
    PlayerHandPtr = ^PlayerHand;
    PlayerHand = array [1..DeckSize] of CardPtr;

procedure InitHand(var h: PlayerHand);
procedure DisposeOfHand(var h: PlayerHand);
function HandIsEmpty(var h: PlayerHand): boolean;
function HandIsOverfilled(var h: PlayerHand): boolean;
function NumCardsInHand(var h: PlayerHand): integer;
procedure TryGetCardFromHand(var h: PlayerHand; var c: CardPtr;
    idx: integer; var success: boolean);
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

procedure DisposeOfHand(var h: PlayerHand);
var
    i: integer;
begin
    for i := 1 to DeckSize do
        if h[i] <> nil then
            dispose(h[i])
end;

function HandIsEmpty(var h: PlayerHand): boolean;
begin
    HandIsEmpty := NumCardsInHand(h) = 0
end;

function HandIsOverfilled(var h: PlayerHand): boolean;
begin
    HandIsOverfilled := NumCardsInHand(h) >= NormalHandSize
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

procedure TryGetCardFromHand(var h: PlayerHand; var c: CardPtr;
    idx: integer; var success: boolean);
var
    i: integer;
begin
    success := false;
    for i := 1 to DeckSize do
        if h[i] <> nil then
            if idx > 1 then
                idx := idx - 1
            else
            begin
                c := h[i];
                success := true;
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

procedure SubstituteCard(var h: PlayerHand;
    PrevCard, NewCard: CardPtr; var success: boolean);
var
    i: integer;
begin
    success := false;
    for i := 1 to DeckSize do
        if h[i] = PrevCard then
        begin
            h[i] := NewCard;
            success := true;
            break
        end
end;

procedure AddCard(var h: PlayerHand; c: CardPtr; var success: boolean);
begin
    success := not CardIsInHand(h, c);
    if success then
        SubstituteCard(h, nil, c, success)
end;

procedure RemoveCard(var h: PlayerHand; c: CardPtr; var success: boolean);
begin
    SubstituteCard(h, c, nil, success)
end;

end.
