using UnityEngine;
using System.Collections;
using System;
using System.Collections.Generic;
using System.Text;

public enum WebServiceType
{
    Regist,
    LoginFromPwd,
    LoginFromToken,
}

[Serializable]
public class RegistUserInfo
{
    public string name;
    public string pwd;
    public RegistUserInfo(string n, string p)
    {
        name = n;
        pwd = p;
    }
}

public class WebServiceManager : MonoBehaviour
{
    public string _svUri = "http://127.0.0.1:8080";
    public string _userId = "admin";
    public string _userPwd = "123456";

    private Dictionary<string, string> _reqHeads = new Dictionary<string, string>();

    void Awake()
    {
        //set auth to head
        string auth = "Basic " + Convert.ToBase64String(Encoding.UTF8.GetBytes(_userId + ":" + _userPwd));
        Debug.Log("Authorization:" + auth);
        _reqHeads.Add("Authorization", auth);
    }

    public void Regist(string name, string pwd, Action<string> onComplete)
    {
        string uJson = JsonUtility.ToJson(new RegistUserInfo(name, pwd));
        byte[] data = Encoding.UTF8.GetBytes(uJson);
        data = PackageData(WebServiceType.Regist, data);
        SendData(data, (t) =>
        {
            if (!string.IsNullOrEmpty(t.error))
            {
                Debug.Log("Regist error:" + t.error);
                if (null != onComplete)
                {
                    onComplete(null);
                }
                return;
            }

            if (null != onComplete)
            {
                onComplete(t.text);
            }
        });
    }

    private byte[] PackageData(WebServiceType type, string token, byte[] data)
    {
        byte[] comb = new byte[33 + data.Length];
        comb[0] = (byte)type;
        Array.Copy(Encoding.UTF8.GetBytes(token), 0, comb, 1, 32);
        Array.Copy(data, 0, comb, 33, data.Length);
        return comb;
    }

    private byte[] PackageData(WebServiceType type, byte[] data)
    {
        byte[] comb = new byte[1 + data.Length];
        comb[0] = (byte)type;
        Array.Copy(data, 0, comb, 1, data.Length);
        return comb;
    }

    private void SendData(byte[] data, Action<WWW> onComplete)
    {
        StartCoroutine(_SendData(data, onComplete));
    }

    private IEnumerator _SendData(byte[] data, Action<WWW> onComplete)
    {
        Dictionary<string, string> heads = new Dictionary<string, string>(_reqHeads);
        WWW w = new WWW(_svUri, data, heads);
        yield return w;
        if (null != onComplete)
        {
            onComplete(w);
        }
    }

    //test
    string name = "";
    string pwd = "";

    void OnGUI()
    {
        name = GUILayout.TextField(name);
        pwd = GUILayout.TextField(pwd);
        if (GUILayout.Button("regist"))
        {
            Regist(name, pwd, (t) =>
            {
                Debug.Log("token:" + t);
            });
        }
    }
}
