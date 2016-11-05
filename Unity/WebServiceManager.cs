using UnityEngine;
using System.Collections;
using System;
using System.Collections.Generic;
using System.Text;

public enum WebServiceType
{
	None,
	Regist,
	LoginFromPwd,
	LoginFromToken
}

public enum StatusCode
{
	Sucesss,
	Error,
	Exception,

	RequestDataFailed,
	DBConnectFailed,
	PwdFailed,
	NameFailed,
	TokenFailed,
	JsonParseFailed,

	RegistUserError,
	LoginUpdateError,
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

[Serializable]
public class LoginFromPwdInfo
{
    public string name;
    public string pwd;
    public LoginFromPwdInfo(string n, string p)
    {
        name = n;
        pwd = p;
    }
}

[Serializable]
public class LoginFromTokenInfo
{
    public string name;
    public string token;
    public LoginFromTokenInfo(string n, string t)
    {
        name = n;
        token = t;
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
        byte[] data = PackageMsg(WebServiceType.Regist, uJson);
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
    public void LoginFromPwd(string name, string pwd, Action<string> onComplete)
    {
        string uJson = JsonUtility.ToJson(new LoginFromPwdInfo(name, pwd));
        byte[] data = PackageMsg(WebServiceType.LoginFromPwd, uJson);
        SendData(data, (t) =>
        {
            if (!string.IsNullOrEmpty(t.error))
            {
                Debug.Log("LoginFromPwd error:" + t.error);
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

    public void LoginFromToken(string name,string token, Action<string> onComplete)
    {
        string uJson = JsonUtility.ToJson(new LoginFromTokenInfo(name, token));
        byte[] data = PackageMsg(WebServiceType.LoginFromToken, uJson);
        SendData(data, (t) =>
        {
            if (!string.IsNullOrEmpty(t.error))
            {
                Debug.Log("LoginFromToken error:" + t.error);
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
    
    public byte[] PackageMsg(WebServiceType type, string config, byte[] data = null)
    {
        byte[] typeByte = BitConverter.GetBytes((Int32)type);
        byte[] configByte = Encoding.UTF8.GetBytes(config);
        byte[] configSizeByte = BitConverter.GetBytes((Int32)configByte.Length);

        int bufSize = typeByte.Length + configSizeByte.Length + configByte.Length;
        if (data != null)
        {
            bufSize += data.Length;
        }

        byte[] buf = new byte[bufSize];
        int offset = 0;

        //消息类型 4 byte
        Array.Copy(typeByte, 0, buf, offset, 4);
        offset += typeByte.Length;

        //消息大小 4 byte
        Array.Copy(configSizeByte, 0, buf, offset, 4);
        offset += configSizeByte.Length;

        //消息
        Array.Copy(configByte, 0, buf, offset, configByte.Length);
        offset += configByte.Length;

        if (data != null)
        {//附加byte[] 数据
            Array.Copy(data, 0, buf, offset, data.Length);
            offset += data.Length;
        }

        if (offset != bufSize)
        {
            Debug.Log("error:offset != bufSize");
        }

        return buf;
    }

    private void SendData(byte[] data, Action<WWW> onComplete)
    {
        Debug.Log("SendData size:" + data.Length);
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
    string token = "";

    void OnGUI()
    {
        GUILayout.BeginHorizontal();
        GUILayout.Label("name:", GUILayout.Width(100));
        name = GUILayout.TextField(name, GUILayout.Width(300));
        GUILayout.EndHorizontal();
        GUILayout.BeginHorizontal();
        GUILayout.Label("pwd:", GUILayout.Width(100));
        pwd = GUILayout.TextField(pwd, GUILayout.Width(300));
        GUILayout.EndHorizontal();
        GUILayout.BeginHorizontal();
        GUILayout.Label("token:", GUILayout.Width(100));
        token = GUILayout.TextField(token, GUILayout.Width(300));
        GUILayout.EndHorizontal();
        if (GUILayout.Button("regist"))
        {
            Regist(name, pwd, (t) =>
            {
                Debug.Log("regist:" + t);
            });
        }
        if (GUILayout.Button("loginFromToken"))
        {
            LoginFromToken(name, token, (t) =>
            {
                Debug.Log("loginFromToken:" + t);
            });
        }
        if (GUILayout.Button("loginFromPwd"))
        {
            LoginFromPwd(name, pwd, (t) =>
            {
                Debug.Log("loginFromPwd:" + t);
            });
        }
    }
}
