# SPDX-FileCopyrightText: 2024-2025 Sony Semiconductor Solutions Corporation
#
# SPDX-License-Identifier: Apache-2.0

# MAINテスト用ビルトインアプリ

## **MAINテスト用ビルトインアプリ使用方法**

1. MAINテスト用ビルトインアプリを使用する際は、configs/<target\>/maverick/kconfig-tweak-for-esf.shを下記の通り編集してください。

* 追加  
  **kconfig-tweak -e CONFIG_EXTERNAL_ENABLE_MAIN_BUILTIN_APP**  

2. FW書き込み後、**esf_main_test** コマンドの引数にテスト番号を指定し任意のテストを実行します。

    > nsh> esf_main_test 0

3. 標準出力にログが表示されます。

## **テスト番号**

  0 : 再起動通知による再起動処理

  1 : シャットダウン通知によるシャットダウン処理

  2 : ファクトリーリセット通知によるファクトリーリセット処理

  3 : SIGTERM 受信による終了処理

  4 : SIGINT 受信による終了処理

  5 : ダミーSystemAppを有効化している場合の再起動処理用テストケース

  6 : ファクトリーリセット(Downgrade)通知によるファクトリーリセット(Downgrade)処理

  7～12 ： 準正常系テスト

## CONFIG

**CONFIG_EXTERNAL_ENABLE_MAIN_BUILTIN_APP**  
MAINテスト用ビルトインアプリを有効化します。

**CONFIG_EXTERNAL_MAIN_ENABLE_LOG**  
ログ制御APIを有効化します。未定義の場合は標準出力にログ出力します。

**CONFIG_EXTERNAL_MAIN_SYSTEMAPP_STUB**  
SystemAppのスタブを有効化します。

有効化する際はSystemApp本体のConfig（CONFIG_EXTERNAL_SYSTEMAPP）を無効化してください。

CONFIG_EXTERNAL_SYSTEMAPPはconfigs/<target\>/maverick/kconfig-tweak-for-app.shに定義されています。

**CONFIG_ETC_ROMFS**  
自動実行を有効化します。

configs/<target\>/maverick/kconfig-tweak-for-evp.shに定義されています。

自動実行でSystemApp起動やIf初期化を行っている場合、main初期化に失敗します。

その場合は本Configを無効化してください。

## DEBUGオプション

設定時は EsfMain の Makefile に定義します。

**ESF_MAIN_DEBUG_LOG_ENABLE**  
Debugレベルログ出力を有効化します。

## 備考

* OS起動時の自動実行でNetwork設定やSystemApp起動を行っていると、mainの起動動作に失敗します。  
  CONFIG_ETC_ROMFSを無効化することで自動実行を無効化して実行するようにしてください。  
  CONFIG_ETC_ROMFSはconfigs/<target\>/maverick/kconfig-tweak-for-evp.shに定義されています。  
