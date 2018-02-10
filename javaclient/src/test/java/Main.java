import java.io.File;
import java.io.FileNotFoundException;
import java.util.Scanner;

/**
 * Created by neil on 2/8/18.
 */
public class Main {

    public static void main(String[] args) throws FileNotFoundException {
        Scanner sc1 = new Scanner(new File("ja.txt"));
        Scanner sc2 = new Scanner(new File("java.txt"));

        for (int j = 0; j < 8; j++) {
            for (int i = 0; i < 52; i++) {
                if (i < 8) {
                    sc1.nextLine();
                    sc2.nextLine();
                } else {
                    String java = sc1.nextLine();
                    String cpp = sc2.nextLine();
                    if (!java.equals(cpp)) {
                        System.out.println("Java doesn't match CPP");
                    }
                }
            }
        }
    }
}
