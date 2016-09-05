package org.liquidplayer.nodedroid;

public class Node {

    private Node() {
        start();
    }

    static {
        System.loadLibrary("node");
        System.loadLibrary("nodedroid");
    }

    private static Node instance = null;

    public static Node getInstance() {
        if (instance==null) {
            instance = new Node();
        }
        return instance;
    }

    public static void exitNode() {
        getInstance().exit();
    }

    @SuppressWarnings("JniMissingFunction")
    private native void start();
    @SuppressWarnings("JniMissingFunction")
    private native void exit();
}
