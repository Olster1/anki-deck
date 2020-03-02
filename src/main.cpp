#include "defines.h"
#include "easy_headers.h"

struct AnkiCard {
    char *question;
    char *answer;
    
    u32 level;
    u64 date;

    bool isVisible;
    
};

struct AnkiDeck {
    int availableCardCound;
    int cardCount;
    int cardsLookedAt;
    AnkiCard cards[2048];
    char *fullFileName;
};


enum AnkiDeck_State {
    ANKI_DECK_QUESTION,
    ANKI_DECK_ANSWER,
    ANKI_DECK_MESSAGE,
    ANKI_DECK_CHOOOSE_DECK,
};

enum AnkiResult {
    ANKI_CORRECT,
    ANKI_INCORRECT, 
};


struct AnkiDeckState {
    u32 cardIndexAt;
    AnkiCard *currentCard; 
    AnkiDeck_State state;
    char *message;

    //NOTE(ollie): Visuals
    Timer textGlowTimer;
};


static void saveAnkiDeck(AnkiDeck *deck) {
    
    InfiniteAlloc fileContents = initInfinteAlloc(char);
    
    for(int i = 0; i < deck->cardCount; ++i) {
        //Get the card out
        AnkiCard *card = &deck->cards[i];
        
        char buffer[32];
        sprintf(buffer, "{\n\n");
        addElementInifinteAllocWithCount_(&fileContents, buffer, strlen(buffer));
        
        
        addVar(&fileContents, card->question, "Question", VAR_CHAR_STAR);
        addVar(&fileContents, card->answer, "Answer", VAR_CHAR_STAR);
        addVar(&fileContents, &card->level, "level", VAR_INT);
        addVar(&fileContents, &card->date, "date", VAR_LONG_UNSIGNED_INT);
        
        sprintf(buffer, "}\n\n");
        addElementInifinteAllocWithCount_(&fileContents, buffer, strlen(buffer));
        
    }
    
    ///////////////////////************ Write the file to disk *************////////////////////
    game_file_handle handle = platformBeginFileWrite(deck->fullFileName);
    platformWriteFile(&handle, fileContents.memory, fileContents.count*fileContents.sizeOfMember, 0);
    platformEndFile(handle);
    
    ///////////////////////************* Clean up the memory ************////////////////////	
    
    releaseInfiniteAlloc(&fileContents);
    
}

static u32 global_myAnki_levelToDaysTable[] = { 0, 1, 1, 5, 12, 24, 48, 96 };


static inline bool shouldAddToPack(u32 level, u64 date) {
    bool result = false;

    u64 now = EasyTime_getTimeStamp();

    assert(date <= now);
    //NOTE(ollie): Get the time in days
    u32 timeInDays = (u32)((float)(now - date)/(60.0f*60.0f*24.0f));

    if(level >= arrayCount(global_myAnki_levelToDaysTable)) {
        level = arrayCount(global_myAnki_levelToDaysTable) - 1;
    }
    u32 daysPassed = global_myAnki_levelToDaysTable[level];

    if(timeInDays >= daysPassed) {
        result = true;
    }
    return result;
    

}

static void loadAnkiDeck(AnkiDeck *deck, char *loadName) {
    
    deck->availableCardCound = 0;
    deck->cardCount = 0;
    deck->cardsLookedAt = 0;

    deck->fullFileName = loadName;

    bool isFileValid = platformDoesFileExist(loadName);
    
    if(isFileValid) {
        
        bool parsing = true;
        FileContents contents = getFileContentsNullTerminate(loadName);
        
        EasyTokenizer tokenizer = lexBeginParsing((char *)contents.memory, EASY_LEX_OPTION_EAT_WHITE_SPACE);
        
        //Default values
        char *question = "NULL";
        char *answer = "NULL";
        u64 date = EasyTime_getTimeStamp();
        u32 level = 0;
        
        while(parsing) {
            EasyToken token = lexGetNextToken(&tokenizer);
            InfiniteAlloc data = {};
            switch(token.type) {
                case TOKEN_NULL_TERMINATOR: {
                    parsing = false;
                } break;
                case TOKEN_WORD: {
                    if(stringsMatchNullN("Question", token.at, token.size)) {
                        
                        question = getStringFromDataObjects(&data, &tokenizer);
                    }
                    if(stringsMatchNullN("Answer", token.at, token.size)) {
                        answer = getStringFromDataObjects(&data, &tokenizer);
                    }
                    if(stringsMatchNullN("date", token.at, token.size)) {
                        date = getULongFromDataObjects(&data, &tokenizer);
                    }
                    if(stringsMatchNullN("level", token.at, token.size)) {
                        level = getIntFromDataObjects(&data, &tokenizer);
                    }
                    
                    
                } break;
                case TOKEN_CLOSE_BRACKET: {
                    if(deck->cardCount < arrayCount(deck->cards)) {


                        AnkiCard *card = deck->cards + deck->cardCount++;
                        
                        card->question = question;
                        card->answer = answer;
                        card->date = date;
                        card->level = level;

                        if(shouldAddToPack(level, date)) {
                            card->isVisible = true;
                            deck->availableCardCound++;
                        } else {
                            card->isVisible = false;
                        }
                        
                        //Default values
                        char *question = "NULL";
                        char *answer = "NULL";
                        u64 date = EasyTime_getTimeStamp();
                        u32 level = 0;
                        
                    } else {
                        parsing = false;
                    }
                } break;
                case TOKEN_OPEN_BRACKET: {
                    
                } break;
                default: {
                    
                }
            }
        }
        easyFile_endFileContents(&contents);
    }
    
    
}

static void updateCard(AnkiDeck *deck, AnkiCard *card, AnkiResult result) {
    if(result == ANKI_CORRECT) {
        card->level++;
        card->date = EasyTime_getTimeStamp();

        //NOTE(ollie): Make sure it doesn't go over the array count
        card->level = min(card->level, arrayCount(global_myAnki_levelToDaysTable) - 1);
    } else {
        assert(result == ANKI_INCORRECT);
        //NOTE(ollie): Incorrect so go down a level
        card->level = max(0, card->level - 1);

        card->date = EasyTime_getTimeStamp();
    }

    saveAnkiDeck(deck);
}

static void drawStringInMiddle(Font *font, char *string, V2 resolution, float screenRelativeSize) {
    Rect2f m = rect2fMinDim(0.15f*resolution.x, 0.15f*resolution.y, 0.7f*resolution.x, 0.7f*resolution.y);
    V2 bounds = getBounds(string, m, font, 1, resolution, screenRelativeSize);
    m = rect2fMinDim(0.5f*resolution.x - 0.5f*bounds.x, 0.5f*resolution.y - 0.5f*bounds.y - font->fontHeight, bounds.x, bounds.y + 3*font->fontHeight);
    outputTextNoBacking(font, m.min.x, m.min.y + font->fontHeight, NEAR_CLIP_PLANE, resolution, string, m, COLOR_BLACK, 1, true, screenRelativeSize);
}

static void goToNextCard(AnkiDeckState *state, AnkiDeck *deck) {
    bool foundCard = false;

    while((state->cardIndexAt + 1) < deck->cardCount && !foundCard) {
        state->cardIndexAt++;

        AnkiCard *nextCard = &deck->cards[state->cardIndexAt];

        if(nextCard->isVisible) {
            state->state = ANKI_DECK_QUESTION;
            state->currentCard = nextCard;
            foundCard = true;
            deck->cardsLookedAt++;
            break;
        }
            
    }

    if(!foundCard) {
        state->currentCard = 0;
        state->state = ANKI_DECK_MESSAGE;
        state->message = "You've got no more cards to review.";
        state->cardIndexAt = 0;
    }

}

int main(int argc, char *args[]) {
    DEBUG_TIME_BLOCK_FOR_FRAME_BEGIN(beginFrame, "Main: Intial setup");
    
    if(argc > 1) {
        for(int i = 0; i < argc; i++) {
            if(cmpStrNull("shaders", args[i])) {
                globalDebugWriteShaders = true;    
            }
            
        }
    }
    
    V2 screenDim = v2(DEFINES_WINDOW_SIZE_X, DEFINES_WINDOW_SIZE_Y); //init in create app function
    V2 resolution = v2(DEFINES_RESOLUTION_X, DEFINES_RESOLUTION_Y);
    
    // screenDim = resolution;
    bool fullscreen = false;
    OSAppInfo appInfo = easyOS_createApp("My Anki Deck", &screenDim, fullscreen);
    
    if(appInfo.valid) {
        EasyTime_setupTimeDatums();
        
        
        easyOS_setupApp(&appInfo, &resolution, RESOURCE_PATH_EXTENSION);
        
        
        ////INIT FONTS
        char *fontName = concatInArena(globalExeBasePath, "/fonts/UbuntuMono-Regular.ttf", &globalPerFrameArena);
        Font mainFont = initFont(fontName, 88);
        Font smallMainFont = initFont(fontName, 52);
        
        char *fontName1 = concatInArena(globalExeBasePath, "/fonts/UbuntuMono-Regular.ttf", &globalPerFrameArena);
        
        //Set the debug font 
        globalDebugFont = initFont(fontName1, 32);
        ///
        
        
        
        //******** CREATE THE FRAME BUFFERS ********///
        
        FrameBuffer mainFrameBuffer = createFrameBuffer(resolution.x, resolution.y, FRAMEBUFFER_COLOR | FRAMEBUFFER_DEPTH | FRAMEBUFFER_STENCIL | FRAMEBUFFER_HDR, 2);
        
        //////////////////////////////////////////////////
        
        
        /////************** Engine requirments *************//////////////
        
        bool hasBlackBars = true;
        bool running = true;
        AppKeyStates keyStates = {};
        
        
        EasyConsole console = {};
        easyConsole_initConsole(&console, BUTTON_TILDE);
        
        DEBUG_globalEasyConsole = &console;
        
        easyFlashText_initManager(&globalFlashTextManager, &mainFont, resolution, appInfo.screenRelativeSize);
        
        
        
        EasyProfile_ProfilerDrawState *profilerState = EasyProfiler_initProfiler(); 
        
        
        ///////////************************/////////////////
        AnkiDeckState state = {};
        state.state = ANKI_DECK_CHOOOSE_DECK;

        state.textGlowTimer = initTimer(3.0f, false);

        // Texture outlineImage =  loadImage(concatInArena(globalExeBasePath, "outlineTextBox.png", &globalPerFrameArena), TEXTURE_FILTER_LINEAR, true);
        Texture spaceImage =  loadImage(concatInArena(globalExeBasePath, "forest.bmp", &globalPerFrameArena), TEXTURE_FILTER_LINEAR, true);

        char *fileTypes[] = {"deck"};
        FileNameOfType deckNames = getDirectoryFilesOfType(concatInArena(globalExeBasePath, "../decks/", &globalPerFrameArena), fileTypes, arrayCount(fileTypes));
        int deckCount = deckNames.count;
            
        AnkiDeck deck = {};

        while(running) {
            
            easyOS_processKeyStates(&keyStates, resolution, &screenDim, &running, !hasBlackBars);
            easyOS_beginFrame(resolution, &appInfo);
            
            beginRenderGroupForFrame(globalRenderGroup);
            
            clearBufferAndBind(appInfo.frameBackBufferId, COLOR_BLACK, FRAMEBUFFER_COLOR, 0);
            
            clearBufferAndBind(mainFrameBuffer.bufferId, COLOR_WHITE, mainFrameBuffer.flags, globalRenderGroup);
            
            renderEnableDepthTest(globalRenderGroup);
            renderEnableCulling(globalRenderGroup);
            setBlendFuncType(globalRenderGroup, BLEND_FUNC_STANDARD_PREMULTIPLED_ALPHA);
            
            renderSetViewPort(0, 0, resolution.x, resolution.y);
            
            /////////////// Write Code Here /////////////////////
            Font *font = &smallMainFont;


            float aspectRatio = (float)spaceImage.height / (float)spaceImage.width;
            float xWidth = resolution.x;
            float xHeight = xWidth*aspectRatio;

            renderTextureCentreDim(&spaceImage, v3(0, 0, 10), v2(xWidth, xHeight), COLOR_WHITE, 0, mat4(), mat4(),  OrthoMatrixToScreen(resolution.x, resolution.y));                

            renderDrawRect(rect2fMinDim(0.1f*resolution.x, 0.1f*resolution.y, 0.8f*resolution.x, 0.8f*resolution.y), 8, COLOR_WHITE, 0, mat4(), OrthoMatrixToScreen_BottomLeft(resolution.x, resolution.y));                    
            
            static s32 deckIndexAt = 0;

            if(wasPressed(keyStates.gameButtons, BUTTON_ESCAPE)) {
                //NOTE(ollie): Go back to choosing a deck
                deck.cardCount = 0;
                deck.availableCardCound = 0;
                state.state = ANKI_DECK_CHOOOSE_DECK;
                deckIndexAt = 0;
            }

            switch(state.state) {
                case ANKI_DECK_CHOOOSE_DECK: {
                    if(deckCount == 0) {
                        state.message = "No decks found. Look at README to see how to add deck.";
                        state.state = ANKI_DECK_MESSAGE;
                    } else {


                        float xAt = 0.2f*resolution.x;
                        float yAt = 0.2f*resolution.y;

                        

                        //NOTE(ollie): Navigate the decks
                        if(wasPressed(keyStates.gameButtons, BUTTON_DOWN)) {
                            deckIndexAt++;
                            if(deckIndexAt >= deckCount) {
                                deckIndexAt--;
                            } else {
                                turnTimerOn(&state.textGlowTimer);
                            }
                        }

                        if(wasPressed(keyStates.gameButtons, BUTTON_UP)) {
                            deckIndexAt--;
                            if(deckIndexAt < 0) {
                                deckIndexAt++;
                            } else {
                                turnTimerOn(&state.textGlowTimer);
                            }
                        }

                        if(!isOn(&state.textGlowTimer) ) {
                            turnTimerOn(&state.textGlowTimer);
                        }

                        assert(isOn(&state.textGlowTimer));

                        TimerReturnInfo timeInfo = updateTimer(&state.textGlowTimer, appInfo.dt);
                            
                        if(timeInfo.finished) {
                            turnTimerOn(&state.textGlowTimer);
                        }
                        

                        ////////////////////////////////////////////////////////////////////

                        ///////////////////////*********** Loop trhough the decks & draw the text**************////////////////////
                        for(int i = 0; i < deckCount; ++i) {
                            char *fullName = deckNames.names[i];
                            char *shortName = getFileLastPortion(fullName);
                            if(shortName[0] != '.') { //don't load hidden file 

                                Rect2f m = rect2fMinDim(0, 0, resolution.x, resolution.y);
                                Rect2f choiceBounds = outputTextNoBacking(font, xAt, yAt, NEAR_CLIP_PLANE, resolution, shortName, m, COLOR_BLACK, 1, false, appInfo.screenRelativeSize);

                                // renderTextureCentreDim(&outlineImage, v3(100, 100, NEAR_CLIP_PLANE + 0.1f), v2(0.8f*resolution.x, 1.5f*getDim(choiceBounds).y), COLOR_WHITE, 0, mat4TopLeftToBottomLeft(resolution.y), mat4(),  OrthoMatrixToScreen(resolution.x, resolution.y));                
                                

                                if(inBounds(keyStates.mouseP, choiceBounds, BOUNDS_RECT)) {
                                    deckIndexAt = i;
                                }

                                V4 color = COLOR_BLACK;
                                if(deckIndexAt == i) {


                                    //NOTE(ollie): gold
                                    color = smoothStep01010V4(COLOR_BLACK, timeInfo.canonicalVal, v4(1, 0.9f, 0, 1.0f));
                                    if(wasPressed(keyStates.gameButtons, BUTTON_ENTER) || wasPressed(keyStates.gameButtons, BUTTON_LEFT_MOUSE)) {
                                        //NOTE(ollie): Load the deck for the first time
                                        loadAnkiDeck(&deck, fullName);

                                        //NOTE(ollie): Set the current card to the first card
                                        if(deck.cardCount) {
                                            if(deck.availableCardCound > 0) {
                                                bool looking = true;
                                                for(int crdIndex = 0; crdIndex < deck.cardCount && looking; ++crdIndex) {
                                                    state.currentCard = &deck.cards[crdIndex];
                                                    state.cardIndexAt = crdIndex;
                                                    state.state = ANKI_DECK_QUESTION;  

                                                    if(state.currentCard->isVisible) {
                                                        //NOTE(ollie): We found a visible card
                                                        looking = false;
                                                        break;
                                                    }  
                                                }
                                                
                                            } else {
                                                state.message = "There aren't any cards that need reviewing.\n\nPress 'Escape' to get back.";
                                                state.state = ANKI_DECK_MESSAGE;
                                            }
                                            
                                        } else {
                                            state.message = "There aren't any cards in this deck.\n\nPress 'Escape' to get back.";
                                            state.state = ANKI_DECK_MESSAGE;
                                        }
                                    }
                                } 

                                outputTextNoBacking(font, xAt, yAt, NEAR_CLIP_PLANE, resolution, shortName, m, color, 1, true, appInfo.screenRelativeSize);

                                float margin = 0.05f*resolution.y;
                                yAt += getDim(choiceBounds).y + margin;  

                            }

                            free(shortName);
                        }

                        ////////////////////////////////////////////////////////////////////
                    }
                    
                } break;
                case ANKI_DECK_QUESTION: {
                    if(state.currentCard) {
                        AnkiCard *card = state.currentCard; 
                        
                        drawStringInMiddle(font, card->question, resolution, appInfo.screenRelativeSize);

                        if(wasReleased(keyStates.gameButtons, BUTTON_SPACE) || wasPressed(keyStates.gameButtons, BUTTON_LEFT_MOUSE)) {
                            state.state = ANKI_DECK_ANSWER; 
                        }

                        char cardOverview[256];
                        sprintf(cardOverview, "%d/%d", deck.cardsLookedAt, deck.availableCardCound);
                        outputTextNoBacking(font, 0.75f*resolution.x, 0.2f*resolution.y, NEAR_CLIP_PLANE, resolution, cardOverview, InfinityRect2f(), COLOR_BLACK, 1, true, appInfo.screenRelativeSize);
                    } else {
                        state.state = ANKI_DECK_MESSAGE;
                        state.message = "There aren't any cards in the deck";
                    }
                } break;
                case ANKI_DECK_ANSWER: {
                    if(state.currentCard) {
                        AnkiCard *card = state.currentCard; 
                        
                        drawStringInMiddle(font, card->answer, resolution, appInfo.screenRelativeSize);

                        ////////////////////////////////////////////////////////////////////
                        Rect2f m = rect2fMinDim(0, 0, resolution.x, resolution.y);

                        Rect2f yesBounds = outputTextNoBacking(font, 0.2f*resolution.x, 0.8f*resolution.y, NEAR_CLIP_PLANE, resolution, "Got it!", m, COLOR_BLACK, 1, true, appInfo.screenRelativeSize);

                        Rect2f noBounds = outputTextNoBacking(font, 0.65f*resolution.x, 0.8f*resolution.y, NEAR_CLIP_PLANE, resolution, "Missed!", m, COLOR_BLACK, 1, true, appInfo.screenRelativeSize);

                        V4 color = COLOR_GREY;

                        char cardOverview[256];
                        sprintf(cardOverview, "%d/%d", deck.cardsLookedAt, deck.availableCardCound);
                        outputTextNoBacking(font, 0.75f*resolution.x, 0.2f*resolution.y, NEAR_CLIP_PLANE, resolution, cardOverview, InfinityRect2f(), COLOR_BLACK, 1, true, appInfo.screenRelativeSize);

                        float myMargin = 50;
                        //NOTE(ollie): Expand the rectangles
                        yesBounds = expandRectf(yesBounds, v2(myMargin, myMargin));

                        noBounds = expandRectf(noBounds, v2(myMargin, myMargin));

                        if(inBounds(keyStates.mouseP, yesBounds, BOUNDS_RECT)) {
                            color = COLOR_YELLOW;
                            if(wasPressed(keyStates.gameButtons, BUTTON_LEFT_MOUSE)) {
                                updateCard(&deck, card, ANKI_CORRECT);
                                goToNextCard(&state, &deck);

                            }
                        }

                        //NOTE(ollie): Draw the quad
                        renderDrawRect(yesBounds, NEAR_CLIP_PLANE + 0.01f, color, 0, mat4TopLeftToBottomLeft(resolution.y), OrthoMatrixToScreen_BottomLeft(resolution.x, resolution.y));                    

                        //NOTE(ollie): Reset the color
                         color = COLOR_GREY;

                        if(inBounds(keyStates.mouseP, noBounds, BOUNDS_RECT)) {
                            color = COLOR_YELLOW;
                            if(wasPressed(keyStates.gameButtons, BUTTON_LEFT_MOUSE)) {
                                updateCard(&deck, card, ANKI_INCORRECT);
                                goToNextCard(&state, &deck);
                            }
                        }


                        //NOTE(ollie): Draw the quad
                        renderDrawRect(noBounds, NEAR_CLIP_PLANE + 0.01f, color, 0, mat4TopLeftToBottomLeft(resolution.y), OrthoMatrixToScreen_BottomLeft(resolution.x, resolution.y));                    


                        // //NOTE(ollie): Can quickly say yes hitting space bar
                        // if(wasReleased(keyStates.gameButtons, BUTTON_SPACE)) {
                        //     updateCard(&deck, card, ANKI_CORRECT);

                        //     //NOTE(ollie): Has cards left
                        //     goToNextCard(&state, &deck);
                            
                        // }

                        ////////////////////////////////////////////////////////////////////
                    } else {
                        state.state = ANKI_DECK_MESSAGE;
                        state.message = "There aren't any cards in the deck";
                    }
                } break;
                case ANKI_DECK_MESSAGE: {
                    drawStringInMiddle(font, state.message, resolution, appInfo.screenRelativeSize);
                } break;
                default: {
                    //do nothing
                }
            }

            drawRenderGroup(globalRenderGroup, RENDER_DRAW_SORT); 
            
            ////////////////////////////////////////////////////
            
            easyOS_updateHotKeys(&keyStates);
            easyOS_endFrame(resolution, screenDim, mainFrameBuffer.bufferId, &appInfo, hasBlackBars);
            
            DEBUG_TIME_BLOCK_FOR_FRAME_END(beginFrame, wasPressed(keyStates.gameButtons, BUTTON_F4));
            
            DEBUG_TIME_BLOCK_FOR_FRAME_START(beginFrame, "Per frame");
            
            easyOS_endKeyState(&keyStates);
        }
        easyOS_endProgram(&appInfo);
    }
    return(0);
}
