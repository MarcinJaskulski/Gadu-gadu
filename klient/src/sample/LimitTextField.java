package sample;

import java.util.Objects;
import javafx.beans.property.IntegerProperty;
import javafx.beans.property.SimpleIntegerProperty;
import javafx.scene.control.TextField;

public class LimitTextField extends TextField { //TextField z ograniczoną pojemnością (maxLength)

    private final IntegerProperty maxLength;

    public LimitTextField() {
        super();
        this.maxLength = new SimpleIntegerProperty(-1);
    }

    public IntegerProperty maxLengthProperty() {
        return this.maxLength;
    }

    public final Integer getMaxLength() {
        return this.maxLength.getValue();
    }

    public final void setMaxLength(Integer maxLength) {
        Objects.requireNonNull(maxLength, "Max length cannot be null, -1 for no limit");
        this.maxLength.setValue(maxLength);
    }

    @Override
    public void replaceText(int start, int end, String insertedText) {
        if (this.getMaxLength() <= 0) {
            super.replaceText(start, end, insertedText);
        }
        else {
            String currentText = this.getText() == null ? "" : this.getText();
            String finalText = currentText.substring(0, start) + insertedText + currentText.substring(end);
            int numberOfexceedingCharacters = finalText.length() - this.getMaxLength();
            if (numberOfexceedingCharacters <= 0) {
                // Normal behavior
                super.replaceText(start, end, insertedText);
            }
            else { //ucina końcówkę jeśli długość wykracza poza maxLength
                String cutInsertedText = insertedText.substring(
                        0,
                        insertedText.length() - numberOfexceedingCharacters
                );
                super.replaceText(start, end, cutInsertedText);
            }
        }
    }
}