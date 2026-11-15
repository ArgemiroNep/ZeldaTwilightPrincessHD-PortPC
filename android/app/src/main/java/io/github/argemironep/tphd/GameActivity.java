package io.github.argemironep.tphd;

import org.libsdl.app.SDLActivity;

/**
 * The game: SDL's activity, which loads libmain.so (the port's runtime with the game code you built
 * from your own game) and calls its main(). The runtime keeps everything in the app's external files
 * folder, Android/data/io.github.argemironep.tphd/files: game/ (your extracted game: code, content,
 * meta), save/, config/ and an optional env.txt (README, "Android").
 */
public class GameActivity extends SDLActivity {
    @Override
    protected String[] getLibraries() {
        return new String[] { "c++_shared", "SDL3", "main" };
    }
}
