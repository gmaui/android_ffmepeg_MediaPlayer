package cn.whaley.sh.mediaplayer;

import android.app.ListActivity;
import android.content.Intent;
import android.os.Bundle;
import android.view.View;
import android.widget.ListView;
import android.widget.TextView;

import java.io.File;
import java.util.Arrays;
import java.util.Comparator;

import cn.whaley.sh.mediaplayer.ui.MessageBox;

/**
 * Created by hc on 2016/4/11.
 */
public class FileExplorerActivity extends ListActivity {

    private static final String TAG = "FileExplorerActivity";

    private String mRoot = "/mnt/usb";
    private TextView mTextViewLocation;
    private File[] mFiles;

    public static final String[] EXTENSIONS = new String[] {
            //video format
            ".mp4",
            ".flv",
            ".avi",
            ".wmv",
            ".m2ts",
            ".rmvb",
            ".mpeg",
            //audio format
            ".mp3",
            ".aac",
            ".ac3",
            ".wav",
            ".amr",
            ".mp2",
            ".ogg",
            ".wma"
    };

    @Override
    protected void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);

        setContentView(R.layout.file_explorer);
        mTextViewLocation = (TextView) findViewById(R.id.textview_path);
        getDirectory(mRoot);
    }

    protected static boolean checkExtension(File file) {
        for (int i = 0; i < EXTENSIONS.length; i++) {
            if (file.getName().indexOf(EXTENSIONS[i]) > 0) {
                return true;
            }
        }
        return false;
    }

    private void sortFilesByDirectory(File[] files) {
        Arrays.sort(files, new Comparator<File>() {

            public int compare(File f1, File f2) {
                return Long.valueOf(f1.length()).compareTo(f2.length());
            }

        });
    }

    private void getDirectory(String dirPath) {
        try {
            mTextViewLocation.setText("Location: " + dirPath);

            File f = new File(dirPath);
            File[] temp = f.listFiles();

            sortFilesByDirectory(temp);

            File[] files = null;
            if (!dirPath.equals(mRoot)) {
                files = new File[temp.length + 1];
                System.arraycopy(temp, 0, files, 1, temp.length);
                files[0] = new File(f.getParent());
            } else {
                files = temp;
            }

            mFiles = files;
            setListAdapter(new FileExplorerAdapter(this, files, temp.length == files.length));
        } catch (Exception ex) {
            MessageBox.show(this, "Error", ex.getMessage());
        }
    }

    @Override
    protected void onListItemClick(ListView l, View v, int position, long id) {
        File file = mFiles[position];

        if (file.isDirectory()) {
            if (file.canRead())
                getDirectory(file.getAbsolutePath());
            else {
                MessageBox.show(this, "Error", "[" + file.getName() + "] folder can't be read!");
            }
        } else {
            if (!checkExtension(file)) {
                StringBuilder strBuilder = new StringBuilder();
                for (int i = 0; i < EXTENSIONS.length; i++)
                    strBuilder.append(EXTENSIONS[i] + " ");
                MessageBox.show(this, "Error", "File must have this extensions: " + strBuilder.toString());
                return;
            }

            startPlayer(file.getAbsolutePath());
        }
    }

    private void startPlayer(String filePath) {
        Intent i = new Intent(this, MediaPlayerActivity.class);
        i.putExtra(getResources().getString(R.string.input_file), filePath);
        startActivity(i);
    }

}
