package cn.whaley.sh.mediaplayer.ui;

import android.app.AlertDialog;
import android.content.Context;
import android.content.DialogInterface;

/**
 * Created by hc on 2016/4/11.
 */
public class MessageBox {
    public static void show(Context context, String title, String msg) {
        new AlertDialog.Builder(context)
                .setMessage(msg)
                .setTitle(title)
                .setCancelable(true)
                .setNeutralButton(android.R.string.ok,
                        new DialogInterface.OnClickListener() {
                            public void onClick(DialogInterface dialog, int whichButton){}
                        })
                .show();
    }

    public static void show(Context context, Exception ex) {
        show(context, "Error", ex.getMessage());
    }
}
