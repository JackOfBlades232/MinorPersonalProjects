unit restock; { restock.pp }
{
    Contains functionality for restocking a hand/hands with cards from deck
}
interface
uses deck, hand, card;

procedure InitRestockHands(var h1, h2: PlayerHand;
    var d: CardDeck; var ok: boolean);
procedure RestockHand(var h: PlayerHand; var d: CardDeck);


implementation

procedure TryMoveCardFromDeckToHand(var h: PlayerHand; var d: CardDeck;
    var success: boolean);
var
    c: CardPtr;
begin
    TryTakeTopCard(d, c, success);
    if success then
        AddCard(h, c, success)
end;

procedure RestockHand(var h: PlayerHand; var d: CardDeck);
var
    ok: boolean;
begin
    ok := true;
    while ok and (not HandIsOverfilled(h)) do
        TryMoveCardFromDeckToHand(h, d, ok)
end;

procedure InitRestockHands(var h1, h2: PlayerHand;
    var d: CardDeck; var ok: boolean);
begin
    ok := DeckIsFull(d) and HandIsEmpty(h1) and HandIsEmpty(h2);
    if ok then
    begin
        RestockHand(h1, d);
        RestockHand(h2, d)
    end
end;

end.
