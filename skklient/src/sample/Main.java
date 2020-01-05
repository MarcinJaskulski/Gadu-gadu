package sample;

import javafx.application.Application;
import javafx.application.Platform;
import javafx.fxml.FXMLLoader;
import javafx.geometry.Insets;
import javafx.geometry.Pos;
import javafx.geometry.VPos;
import javafx.scene.Parent;
import javafx.scene.Scene;
import javafx.scene.control.Button;
import javafx.scene.control.Label;
import javafx.scene.control.ScrollPane;
import javafx.scene.control.TextField;
import javafx.scene.layout.*;
import javafx.scene.text.Text;
import javafx.stage.Stage;

import java.io.BufferedReader;
import java.io.InputStreamReader;
import java.io.PrintWriter;
import java.net.Socket;
import java.text.Bidi;
import java.util.*;

import static java.lang.Thread.sleep;

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
    TextField text;
    GridPane sender;
    Button send;
    //data objects

    Map<Integer, Button> friendsDictionary;
    Map<Button, Integer> friendsDictionaryReverse;
    Map<Integer, List<Label>> friendsMessages;
    String buffer;
    Integer id;
    Integer friendId;
    //connection objects
    Socket socket;
    BufferedReader reader;
    PrintWriter writer;

    @Override
    public void start(Stage primaryStage) throws Exception {

        setUpLayout(primaryStage);
        setUpConnection();

        primaryStage.setOnCloseRequest(e -> {
            try {
                writer.print("#end");
                socket.close();
                primaryStage.close();
            } catch (Exception ex) {

                System.out.println(ex.toString());
            }

        });
        Reader r = new Reader(socket, this);
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
//        messages.getChildren().addAll(wiad1,wiad2,wiad3,wiad4);
        messageBox = new ScrollPane();
        messageBox.setContent(messages);
        messageBox.setFitToWidth(true);
        messageBox.vvalueProperty().bind(messages.heightProperty());
//        wiad1.setWrapText(true);
//        wiad2.setWrapText(true);
//        wiad3.setWrapText(true);
//        wiad4.setWrapText(true);
        //wiad1.setWrappingWidth(messages.getWidth());
        col1 = new ColumnConstraints();
        col2 = new ColumnConstraints();
        row1 = new RowConstraints();
        row2 = new RowConstraints();
        row1.setPercentHeight(80);
        row2.setPercentHeight(20);
        col1.setPercentWidth(30);
        col2.setPercentWidth(70);
        text = new TextField();
        text.setOnAction(e->{
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
        send.setOnAction(e->{
            sendMsg();
        });
        send.setMaxWidth(Double.MAX_VALUE);
        HBox.setHgrow(send, Priority.ALWAYS);
        HBox.setHgrow(text, Priority.ALWAYS);
        sender.add(text, 0, 0);
        sender.add(send, 1, 0);
        layout.add(sender, 1, 1);
        primaryStage.setTitle("Hello World");
        primaryStage.setScene(new Scene(layout, 500, 500));
        primaryStage.show();

    }

    private void setUpConnection() {
        try {
            socket = new Socket("localhost", 1234);
            reader = new BufferedReader(new InputStreamReader(socket.getInputStream()));
            writer = new PrintWriter(socket.getOutputStream(), true);
            friendsDictionary = new HashMap<Integer, Button>();
            friendsDictionaryReverse = new HashMap<Button, Integer>();
            friendsMessages = new HashMap<Integer, List<Label>>();
            buffer = reader.readLine();
            buffer = deleteThrash(buffer);
            String number = buffer.replaceAll("\\D+", "");
            id = Integer.parseInt(number);
            buffer = reader.readLine();
            buffer = deleteThrash(buffer);
            updateFriends(buffer);
        } catch (Exception e) {
            System.out.println(e.toString());
        }

    }

    public String deleteThrash(String msg) {
        int start = -1;
        for (int i = 0; i < msg.length(); i++) {
            if (msg.charAt(i) == '#') {
                start = i;
                break;
            }
        }
        if (start != -1) return msg.substring(start);
        else {
            System.out.println(msg);

            return null;
        }
    }

    public void updateFriends(String buf) {
        String number = "";
        String friendsString = buf.substring(9);
        char c;
        Integer tmp;
        List<Integer> friendsList = new ArrayList<Integer>();
        for (int i = 0; i < friendsString.length(); i++) {
            c = friendsString.charAt(i);
            if (Character.isDigit(c)) {
                number += c;
            } else {
                if (number.equals("")) break;
                tmp = Integer.parseInt(number);
                if (tmp != id) {
                    friendsList.add(tmp);
                    if (!friendsDictionary.containsKey(tmp)) {
                        friendsDictionary.put(tmp, new Button("znajomy" + tmp.toString()));
                        friendsMessages.put(tmp, new ArrayList<Label>());
                        friendsDictionaryReverse.put(new Button("znajomy" + tmp.toString()), tmp);
                        friendsDictionary.get(tmp).setMaxWidth(Double.MAX_VALUE);
                        Integer finalTmp = tmp;
                        Platform.runLater(
                                () -> {
                        friendsDictionary.get(finalTmp).setOnAction(e -> {
                            friendId = finalTmp;
                            System.out.println(friendId);
                                        messages.getChildren().clear();
                                        messages.getChildren().addAll(friendsMessages.get(friendId));
//                            messageBox.setVvalue(1D);

                        });
                        friends.getChildren().add(friendsDictionary.get(finalTmp));
                                }
                        );


                    }
                }
                number = "";
            }

        }
        for (Integer item : friendsDictionary.keySet()) {
            if (!friendsList.contains(item)) {
                Platform.runLater(
                        () -> {
                friends.getChildren().remove(friendsDictionary.get(item));
                friendsDictionary.remove(item);
                friendsMessages.remove(item);
                        }
                );
            }
        }
    }

    public void readMsg(String buf) {
        String number = buf.substring(8, 8 + 3);
        String message = buf.substring(21, buf.length() - 1);
        Integer from = Integer.parseInt(number);
        message = "znajomy" + number + ": " + message;
        Label tmp = new Label(message);
        tmp.setWrapText(true);
        friendsMessages.get(from).add(tmp);
        if (friendId==from) {
            Platform.runLater(
                    () -> {
                        messages.getChildren().add(tmp);
                        messageBox.setVvalue(1D);
                    }
            );
        }
    }
    private void sendMsg(){
        String message=text.getText();
        String msg="";
        if (!message.isEmpty()){
            msg+=String.format("%03d",id);
            msg+=String.format("%03d",friendId);
            msg+=text.getText();
            writer.println(msg);
            message="Ty: "+message;
            Label tmp = new Label(message);
            tmp.setWrapText(true);
            friendsMessages.get(friendId).add(tmp);
                Platform.runLater(
                        () -> {
                            messages.getChildren().add(tmp);
//                            messageBox.setVvalue(1D);
                        }
                );

            text.clear();
        }
    }
    public static void main(String[] args) {
        launch(args);
    }
}
