package sample;

import javafx.scene.control.Label;

import java.io.BufferedReader;
import java.io.InputStreamReader;
import java.net.Socket;

public class Reader implements Runnable {
    BufferedReader reader;
    Main main;
    String buffer;
    public Reader(Socket s,Main main){
        try{
        reader = new BufferedReader(new InputStreamReader(s.getInputStream()));
        }
        catch(Exception e){
            System.out.println(e.toString());
        }
        this.main=main;
    }
    @Override
    public void run() {
        try {
            while (true) { //czytanie wiadomości z serwera i wywoływanie odpowiedniej funkcji w main
                buffer = reader.readLine();

                buffer = main.deleteThrash(buffer);
                if(buffer==null) continue;
                System.out.println(buffer);

                if(buffer.length()>=8) {
                    if (buffer.substring(0, 8).equals("#friends")) {
                        main.updateFriends(buffer);
                        continue;
                    }
                }
                if(buffer.length()>=7) {
                    if (buffer.substring(0, 7).equals("#fromId")) {
                        main.readMsg(buffer);
                    }
                }
            }
        }
        catch( Exception e){
            System.out.println(e.toString());
        }

    }

}
