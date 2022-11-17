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
function RemainingDeckSize(var d: CardDeck): integer;
procedure TryTakeTopCard(var d: CardDeck; var c: CardPtr; var success: boolean);
procedure TryGetTrumpSuit(var d: CardDeck;
    var trump: CardSuit; var success: boolean); 
    
implementation

procedure ReconstructAllCards(var arr: DeckContent); forward;

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

            if arr[ArrIndex] <> nil then
                dispose(arr[ArrIndex]);
            new(arr[ArrIndex]);

            arr[ArrIndex]^.suit := AllSuits[i];
            arr[ArrIndex]^.value := AllValues[j]
        end
end;

function RemainingDeckSize(var d: CardDeck): integer;
begin
    RemainingDeckSize := DeckSize - d.TopIndex + 1
end;

procedure TryTakeTopCard(var d: CardDeck; var c: CardPtr; var success: boolean);
begin
    success := RemainingDeckSize(d) > 0;
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
    success := RemainingDeckSize(d) > 0;
    if success then
        trump := d.content[DeckSize]^.suit
end;

end.
