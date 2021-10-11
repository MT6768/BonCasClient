
# BonCasClient

BonCasClient v1.1.1 (http://www2.wazoku.net/2sen/dtvvup/source/BonCasClient(1.1.1).zip) に同梱されているソースコードのミラー。

ビルド環境を Visual Studio 2019 (VS2019) に更新したこと、パッチ ([BonCasClient_fix.diff.txt](http://www2.wazoku.net/2sen/dtvvup/source/BonCasClient_fix.diff.txt)) を当てたこと、この Readme.md を作成したこと以外はオリジナルのままとなっている。  
以下のドキュメントは [BonCasClient.txt](BonCasClient.txt) を現状に合わせて一部改変し、Markdown 形式に書き換えたものである。  
オリジナルの [BonCasClient.txt](BonCasClient.txt) も参照されたい。

### 利用時の注意

BonCasClient のビット数 (32bit or 64bit) は、各種 WinSCard.dll 同様に TVTest などの BonCasClient を利用するソフトに合わせる必要がある。  
さもなければ、ライブラリの読み込みに失敗し、スクランブルの解除に失敗する。

ただし、BonCasClient と BonCasLink (BonCasServer, BonCasService) が異なるビット数になっている事は問題ない。  
BonCasClient と BonCasLink が TCP で通信しているためで、たとえば TVTest と BonCasClient が 64bit 、BonCasLink が 32bit の場合でも正常に読み込める。

----

## BonCasClient.dll

BonCasLink のクライアントインターフェースを実装した DLL。  
EXPORT されている IF は SCardXXXX と型互換の以下9関数 CasLinkXXXX と、
- CasLinkConnectA
- CasLinkConnectW
- CasLinkDisconnect
- CasLinkEstablishContext
- CasLinkFreeMemory
- CasLinkListReadersA
- CasLinkListReadersW
- CasLinkTransmit
- CasLinkReleaseContext

接続先を設定する1関数、

- void SetConfig(const DWORD dwIP, const WORD wPort)

が実装されています。

## ダウンロード

[BonCasClient v1.1.1-patch](https://github.com/mizunoko/BonCasClient/releases/download/v1.1.1-patch/BonCasClient_v1.1.1-patch.zip)

## 設定

接続先は DLL と同じフォルダの BonCasClient.ini を作成して設定することも出来ます。  
書式及びデフォルト設定 ( ini がない場合) は以下の通りです。

    [Server]
    IP=127.0.0.1
    Port=6900

## VC2019 のランタイムが必要です

Visual Studio 2019 の Microsoft Visual C++ 再頒布可能パッケージ  
32bit (x86) 版 … https://aka.ms/vs/16/release/VC_redist.x86.exe  
64bit (x64) 版 … https://aka.ms/vs/16/release/VC_redist.x64.exe  
(使う DLL により必要なランタイムも x86、x64 で異なります。x64 の OS なら両方入れておいても問題ありません)  

また、Windows Update による更新があれば最新にしてください。  
(開発環境側も Windows Update により必要な環境が更新されている場合があります)

## ベースにしたもの

- Multi2Dec Ver.2.10 (http://2sen.dip.jp/dtv/)
- BonCasLink Ver.1.10 z1

## 履歴

- 2011/09/25 … 64bit用DLL追加
- 2010/11/11 … 実験品
