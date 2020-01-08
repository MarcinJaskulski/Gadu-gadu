package sample;

import javafx.application.Application;
import javafx.application.Platform;
import javafx.geometry.Insets;
import javafx.geometry.Pos;
import javafx.scene.Scene;
import javafx.scene.control.Button;
import javafx.scene.control.Label;
import javafx.scene.control.ScrollPane;
import javafx.scene.layout.*;
import javafx.stage.Modality;
import javafx.stage.Stage;

import java.io.BufferedReader;
import java.io.InputStreamReader;
import java.io.PrintWriter;
import java.net.Socket;
import java.util.*;
import java.util.concurrent.Semaphore;

public class Main extends Application {
    //layout objects
    GridPane layout;
    ScrollPane znajomi;
    VBox friends;
    VBox messages;
    ScrollPane messageBox;
    ColumnConstraints col1;
    ColumnConstraints col2;
    RowConstraints row1;
    RowConstraints row2;
    LimitTextField text;
    GridPane sender;
    Button send;
    Semaphore mutex;
    //data objects
    Boolean firstUpdate;
    Map<Integer, Button> friendsDictionary;
    Map<Integer, List<Label>> friendsMessages;
    String buffer;
    Integer id;
    Integer friendId;
    //connection objects
    Socket socket;
    BufferedReader reader;
    PrintWriter writer;
    Stage main;

    @Override
    public void start(Stage primaryStage) throws Exception {
        main = primaryStage;
        setUpLayout(main);
        setUpConnection();

        primaryStage.setOnCloseRequest(e -> {//przy zamknięciu programu zamyka deskryptor
            try {
                socket.close();
                main.close();
            } catch (Exception ex) {

                System.out.println(ex.toString());
            }

        });
        Reader r = new Reader(socket, this); //ustawienie wątku czytająceg
        Thread readThread = new Thread(r);
        readThread.start();
    }

    private void setUpLayout(Stage primaryStage) {
        layout = new GridPane();
        layout.setHgap(10);
        layout.setVgap(10);
        layout.setPadding(new Insets(10, 10, 10, 10));
        friends = new VBox();
        znajomi = new ScrollPane();
        znajomi.setContent(friends);

        messages = new VBox();
        znajomi.setFitToWidth(true);

        messages.setSpacing(10);
        messages.setAlignment(Pos.BOTTOM_LEFT);
        messageBox = new ScrollPane();
        messageBox.setContent(messages);
        messageBox.setFitToWidth(true);
        messageBox.vvalueProperty().bind(messages.heightProperty());
        mutex = new Semaphore(0);
        col1 = new ColumnConstraints();
        col2 = new ColumnConstraints();
        row1 = new RowConstraints();
        row2 = new RowConstraints();
        row1.setPercentHeight(80);
        row2.setPercentHeight(20);
        col1.setPercentWidth(30);
        col2.setPercentWidth(70);
        text = new LimitTextField();
        text.setMaxLength(100);

        text.setOnAction(e -> {
            sendMsg();
        });
        text.setPromptText("Wpisz wiadomość");
        text.setAlignment(Pos.CENTER_LEFT);
        layout.getRowConstraints().addAll(row1, row2);
        layout.getColumnConstraints().addAll(col1, col2);
        layout.add(znajomi, 0, 0, 1, 2);
        layout.add(messageBox, 1, 0);
        sender = new GridPane();
        sender.getColumnConstraints().addAll(col2, col1);
        sender.getRowConstraints().add(row1);
        send = new Button("Wyślij");
        send.setOnAction(e -> {
            sendMsg();
        });
        send.setMaxWidth(Double.MAX_VALUE);
        HBox.setHgrow(send, Priority.ALWAYS);
        HBox.setHgrow(text, Priority.ALWAYS);
        sender.add(text, 0, 0);
        sender.add(send, 1, 0);
        layout.add(sender, 1, 1);
        primaryStage.setTitle("GG");
        primaryStage.setScene(new Scene(layout, 500, 500));
        primaryStage.show();

    }

    private void setUpConnection() {
        try {
            socket = new Socket("localhost", 1234); //połączenie z serwerem
            reader = new BufferedReader(new InputStreamReader(socket.getInputStream()));
            writer = new PrintWriter(socket.getOutputStream(), true);
            friendsDictionary = new HashMap<Integer, Button>(); //idUżytkownika->przycisk
            firstUpdate = true;
            friendsMessages = new HashMap<Integer, List<Label>>(); //idUżytkownika->historia rozmów z nim
            buffer = reader.readLine(); //sprawdzenie stanu serwera
            if (buffer.length() >= 10) {
                if (buffer.substring(0, 10).equals("#busySpace")) {  //jeśli nie ma miejsca, wyświetlany jes alert, a następnie program jest zamykany
                    Platform.runLater(() -> display());

                }
            }
            buffer = deleteThrash(buffer);
            String number = buffer.replaceAll("\\D+", ""); //pierwsza wiadomość z id, pozbywamy się nie-cyfr i mamy id
            id = Integer.parseInt(number);
            buffer = reader.readLine();
            buffer = deleteThrash(buffer);
            friendId = new Integer(-1); //id użyttkownika, z którym czatujemy aktualnie
            updateFriends(buffer); //sprawdzamy kto jest na serwerze
        } catch (Exception e) {
            System.out.println(e.toString());
        }

    }

    public String deleteThrash(String msg) { //pozbycie się śmieci z wiadomości
        int start = -1;
        for (int i = 0; i < msg.length(); i++) {
            if (msg.charAt(i) == '#') { //każda wiadomość z serwera zaczyna się od #
                start = i;
                break;
            }
        }
        if (start != -1) return msg.substring(start);
        else {
            return null;
        }
    }

    public void updateFriends(String buf) throws InterruptedException {
        String number = "";
        String friendsString = buf.substring(9);
        char c;
        Integer tmp;
        List<Integer> friendsList = new ArrayList<Integer>();
        for (int i = 0; i < friendsString.length(); i++) {
            c = friendsString.charAt(i);
            if (Character.isDigit(c)) {
                number += c; //tworzymy kolejne liczby
            } else {
                if (number.equals("")) break;
                tmp = Integer.parseInt(number);
                if (tmp != id) {  //jeśli dana liczba nie jest naszym id
                    if (firstUpdate) { //ustawiamy friendId jeśli wcześniej nie było
                        friendId = tmp;
                        firstUpdate = false;
                    }
                    friendsList.add(tmp);
                    if (!friendsDictionary.containsKey(tmp)) {
                        friendsDictionary.put(tmp, new Button("znajomy" + tmp.toString())); //dodajemy przycisk oraz historię rozmów dla nowego użytkownika
                        friendsMessages.put(tmp, new ArrayList<Label>());
                        friendsDictionary.get(tmp).setMaxWidth(Double.MAX_VALUE);
                        Integer finalTmp = tmp;

                        friendsDictionary.get(finalTmp).setOnAction(e -> { //przy kliknięciu w przycisk zmienia nam się friendId oraz wyświetlana historia rozmów
                            friendId = finalTmp;
                            messages.getChildren().clear();
                            messages.getChildren().addAll(friendsMessages.get(friendId));
                        });
                        Platform.runLater( //dodajemy przycisk do layoutu
                                () -> {
                                    Integer addButton = new Integer(finalTmp);

                                    friends.getChildren().add(friendsDictionary.get(addButton));
                                }
                        );
                    }
                }
                number = "";
            }

        }
        Iterator<Integer> itr = friendsDictionary.keySet().iterator();
        while (itr.hasNext()) { //usuwamy nieaktywnych użytkowników
            Integer item = itr.next();
            if (!friendsList.contains(item)) {
                Platform.runLater(
                        () -> {
                            friends.getChildren().remove(friendsDictionary.get(item)); //usuwamy przycisk
                            mutex.release(1); //synchronizacja layoutu z innymi zmiennymi
                        }
                );
                mutex.acquire(1);
                if (friendId == item) {
                    Platform.runLater(
                            () -> {
                                messages.getChildren().removeAll(friendsMessages.get(item)); //usuwamy wiadomości jeśli to okno akurat było otwarte
                                mutex.release(1);

                            }
                    );
                    mutex.acquire(1);
                    friendsMessages.remove(item); //usuwamy wiadomości z pamięci
                    if (friendsMessages.keySet().toArray().length > 0) {
                        friendId = (Integer) friendsMessages.keySet().toArray()[0];

                        Platform.runLater(
                                () -> {
                                    messages.getChildren().addAll(friendsMessages.get(friendId)); //zmieniamy wyświetlane wiadomości jeśli jest na co zmienić

                                }
                        );
                    } else {
                        friendId = -1;
                        friendsMessages.remove(item);
                    }

                }
                itr.remove(); //usuwamy przycisk z pamięci
                break;

            }
        }
    }

    public void readMsg(String buf) { //odczytaną wiadomość dodajemy do layoutu,jeśli ta rozmowa jest otwarta
        String number = buf.substring(8, 8 + 3);
        String message = buf.substring(21);
        Integer from = Integer.parseInt(number);
        message = "znajomy" + number + ": " + message;
        Label tmp = new Label(message);
        tmp.setWrapText(true);
        friendsMessages.get(from).add(tmp);
        if (friendId == from) {
            Platform.runLater(
                    () -> {
                        messages.getChildren().add(tmp);
                    }
            );
        }
    }

    public void display() { //wyświetlanie alertu o zapełnionym serwerze
        Stage window = new Stage();
        window.initModality(Modality.APPLICATION_MODAL);
        window.setTitle("Alert");
        window.setMinWidth(250);
        Label text = new Label();
        text.setText("Serwer jest pełny, spróbuj później");
        Button close = new Button("Zamknij");
        close.setOnAction(e -> {
            try {
                socket.close();
            } catch (Exception f) {
                System.out.println(f.toString());
            }
            window.close();
            main.close();
        });
        VBox layout = new VBox();
        layout.getChildren().addAll(text, close);
        layout.setAlignment(Pos.CENTER);
        Scene scene = new Scene(layout, 300, 250);
        window.setScene(scene);
        window.showAndWait();

    }

    private void sendMsg() {
        if (friendId != -1) {
            String message = text.getText(); //pobieramy wiadomość z pola tekstowego
            String msg = "";
            if (!message.isEmpty()) { // jeśli nie jest pusta, doklejamy informacje dla serwera
                msg += String.format("%03d", id);
                msg += String.format("%03d", friendId);
                msg += text.getText();
                writer.println(msg);
                message = "Ty: " + message;
                Label tmp = new Label(message);
                tmp.setWrapText(true);
                friendsMessages.get(friendId).add(tmp); //dodajemy do historii rozmów
                Platform.runLater(
                        () -> {
                            messages.getChildren().add(tmp);
                        }
                );

                text.clear();
            }
        }
    }

    public static void main(String[] args) {
        launch(args);
    }
}
