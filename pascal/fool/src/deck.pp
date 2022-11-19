unit deck; { deck.pp }
{
    contains data structure and functionality for a card deck
}
interface
uses card;
const
    DeckSize = 36;
type
    DeckContent = array [1..DeckSize] of CardPtr;
    CardDeck = record
        content: DeckContent;
        TopIndex: integer;
    end;

procedure GenerateDeck(var d: CardDeck);
function DeckIsEmpty(var d: CardDeck): boolean;
function DeckIsFull(var d: CardDeck): boolean;
function RemainingDeckSize(var d: CardDeck): integer;
procedure TryTakeTopCard(var d: CardDeck; var c: CardPtr; var success: boolean);
procedure TryGetTrumpSuit(var d: CardDeck;
    var trump: CardSuit; var success: boolean); 

    
implementation

procedure ReconstructAllCards(var arr: DeckContent); forward;
procedure RecreateCard(var c: CardPtr; SuitIdx, ValueIdx: integer); forward;

procedure GenerateDeck(var d: CardDeck);
var
    i, j: integer;
    tmp: CardPtr;
begin
    ReconstructAllCards(d.content);
    for i := 1 to DeckSize - 1 do
    begin
        j := i + random(DeckSize - i + 1);

        tmp := d.content[i];
        d.content[i] := d.content[j];
        d.content[j] := tmp
    end;
    d.TopIndex := 1
end;

procedure ReconstructAllCards(var arr: DeckContent);
var
    i, j, ArrIndex: integer;
begin
    for i := 1 to NumberOfSuits do
        for j := 1 to NumberOfValues do
        begin
            ArrIndex := (i - 1) * NumberOfValues + j;
            RecreateCard(arr[ArrIndex], i, j)
        end
end;

procedure RecreateCard(var c: CardPtr; SuitIdx, ValueIdx: integer);
begin
    if c <> nil then
        dispose(c);
    new(c);
    c^.suit := AllSuits[SuitIdx];
    c^.value := AllValues[ValueIdx]
end;

function DeckIsEmpty(var d: CardDeck): boolean;
begin
    DeckIsEmpty := RemainingDeckSize(d) = 0
end;

function DeckIsFull(var d: CardDeck): boolean;
begin
    DeckIsFull := RemainingDeckSize(d) = DeckSize
end;

function RemainingDeckSize(var d: CardDeck): integer;
begin
    RemainingDeckSize := DeckSize - d.TopIndex + 1
end;

procedure TryTakeTopCard(var d: CardDeck; var c: CardPtr; var success: boolean);
begin
    success := not DeckIsEmpty(d);
    if success then
    begin
        c := d.content[d.TopIndex];
        d.content[d.TopIndex] := nil;
        d.TopIndex := d.TopIndex + 1
    end
end;

procedure TryGetTrumpSuit(var d: CardDeck;
    var trump: CardSuit; var success: boolean); 
begin
    success := not DeckIsEmpty(d);
    if success then
        trump := d.content[DeckSize]^.suit
end;

end.
