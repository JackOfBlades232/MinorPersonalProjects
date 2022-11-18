unit restock; { restock.pp }
{
    Contains functionality for restocking a hand/hands with cards from deck
}
interface
uses deck, hand, card;

procedure RestockHand(var h: PlayerHand; var d: CardDeck);
procedure InitFillHands(var h1, h2: PlayerHand;
    var d: CardDeck; var ok: boolean);


implementation

procedure RestockHand(var h: PlayerHand; var d: CardDeck);
var
    ok: boolean;
    c: CardPtr;
begin
    ok := true;
    while ok and (NumCardsInHand(h) < NormalHandSize) do
    begin
        TryTakeTopCard(d, c, ok);
        if ok then
            AddCard(h, c, ok)
    end
end;

procedure InitFillHands(var h1, h2: PlayerHand;
    var d: CardDeck; var ok: boolean);
var
    i: integer;
    c: CardPtr;
begin
    ok := true;
    if (RemainingDeckSize(d) < DeckSize) or 
        (NumCardsInHand(h1) > 0) or (NumCardsInHand(h1) > 0) then
    begin
        ok := false;
        exit
    end;
    for i := 1 to NormalHandSize do
    begin
        TryTakeTopCard(d, c, ok);
        if not ok then
            exit;
        AddCard(h1, c, ok);
        if not ok then
            exit;
        TryTakeTopCard(d, c, ok);
        if not ok then
            exit;
        AddCard(h2, c, ok);
        if not ok then
            exit
    end
end;

end.
