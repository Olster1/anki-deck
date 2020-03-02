This is a very basic anki deck for spaced reviewing. 

To create a deck, create a .deck file (has to be .deck) in the same folder as the program. 
When you open the program, you'll get to choose the deck you want to review, & the cards that need reviewing will
be added to your deck. If you don't have any, that is enough time hasn't passed, it will tell you.

Controls:
SpaceBar to go to the answer
Escape to go back to choosing decks
Enter to choose a deck

Example deck:

{

	Question: "Is this the first card?";
	Answer: "Yes it is";
}
{

	Question: "Is this the second card?";
	Answer: "Yes it is";
}

Once you've revied a deck it will add two extra bits of information: level & date

{

	Question: "Is this the second card?";
	Answer: "Yes it is";
	level: 0;
	date: 1583111578;
}

Ignore these if you add more cards later. You just have to put a question & answer. 