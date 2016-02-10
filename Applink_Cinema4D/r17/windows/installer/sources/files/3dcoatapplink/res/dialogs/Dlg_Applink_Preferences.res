// C4D-DialogResource
DIALOG DLG_APPLINK_PREFERENCES
{
  NAME IDS_APPLINK_PREFERENCES_TITLE; ALIGN_TOP; SCALE_H; 
  
  TAB IDC_STATIC
  {
    NAME IDS_STATIC15; ALIGN_TOP; SCALE_H; 
    SELECTION_TABS; 
    
    GROUP IDS_STATIC13
    {
      NAME IDS_STATIC13; ALIGN_TOP; SCALE_H; 
      BORDERSIZE 0, 0, 0, 0; 
      COLUMNS 1;
      SPACE 4, 4;
      
      GROUP IDC_STATIC
      {
        NAME IDS_STATIC1; ALIGN_TOP; SCALE_H; 
        BORDERSTYLE BORDER_GROUP_IN; BORDERSIZE 10, 4, 10, 4; 
        COLUMNS 1;
        SPACE 4, 4;
        
        GROUP IDC_STATIC
        {
          NAME IDS_STATIC11; ALIGN_TOP; SCALE_H; 
          BORDERSIZE 0, 0, 0, 0; 
          COLUMNS 2;
          SPACE 4, 4;
          
          STATICTEXT IDC_STATIC { NAME IDS_TMP_FOLDER; CENTER_V; ALIGN_LEFT; }
          FILENAME IDC_TMP_FOLDER
          {
            ALIGN_TOP; SCALE_H; 
            DIRECTORY; 
          }
          STATICTEXT IDC_STATIC { NAME IDS_EXCH_FOLDER; CENTER_V; ALIGN_LEFT; }
          FILENAME IDC_EXCH_FOLDER
          {
            ALIGN_TOP; SCALE_H; 
            DIRECTORY; 
          }
        }
        STATICTEXT IDC_STATIC { NAME IDS_STATIC12; CENTER_V; ALIGN_LEFT; }
      }
      GROUP IDC_STATIC
      {
        NAME IDS_STATIC2; ALIGN_TOP; SCALE_H; 
        BORDERSTYLE BORDER_GROUP_IN; BORDERSIZE 10, 4, 10, 4; 
        COLUMNS 1;
        SPACE 4, 4;
        
        GROUP IDC_STATIC
        {
          NAME IDS_STATIC19; ALIGN_TOP; SCALE_H; 
          BORDERSIZE 0, 0, 0, 0; 
          COLUMNS 2;
          SPACE 4, 4;
          
          STATICTEXT IDC_STATIC { NAME IDS_STATIC20; CENTER_V; ALIGN_LEFT; }
          COMBOBOX IDC_COMBO_MAP_TYPE
          {
            ALIGN_TOP; SCALE_H; SIZE 150, 0; 
            CHILDS
            {
              0, STR_MAP_ID0; 
              1, STR_MAP_ID1; 
              2, STR_MAP_ID2; 
              3, STR_MAP_ID3; 
              4, STR_MAP_ID4; 
              5, STR_MAP_ID5; 
              6, STR_MAP_ID6; 
              7, STR_MAP_ID7; 
              8, STR_MAP_ID8; 
              9, STR_MAP_ID9; 
              10, STR_MAP_ID10; 
            }
          }
        }
        CHECKBOX IDC_CHK_EXP_MAT { NAME IDS_CHK_EXP_MAT; ALIGN_TOP; ALIGN_LEFT;  }
        CHECKBOX IDC_CHK_EXP_UV { NAME IDS_CHK_EXP_UV; ALIGN_TOP; ALIGN_LEFT;  }
		CHECKBOX IDC_CHK_SKIP_IMP_DIALOG { NAME IDS_CHK_SKIP_IMP_DIALOG; ALIGN_TOP; ALIGN_LEFT;  }
		CHECKBOX IDC_CHK_SKIP_EXP_DIALOG{ NAME IDS_CHK_SKIP_EXP_DIALOG; ALIGN_TOP; ALIGN_LEFT;  }
        BUTTON IDC_BTN_EXPORT { NAME IDS_BTN_EXPORT; ALIGN_TOP; SCALE_H; }
      }
      GROUP IDC_STATIC
      {
        NAME IDS_STATIC3; ALIGN_TOP; SCALE_H; 
        BORDERSTYLE BORDER_GROUP_IN; BORDERSIZE 10, 4, 10, 4; 
        COLUMNS 1;
        SPACE 4, 4;
        
        CHECKBOX IDC_CHK_IMP_MAT { NAME IDS_CHK_IMP_MAT; ALIGN_TOP; ALIGN_LEFT;  }
        GROUP IDC_STATIC
        {
          NAME IDS_STATIC21; ALIGN_TOP; SCALE_H; 
          BORDERSIZE 0, 0, 0, 0; 
          COLUMNS 2;
          SPACE 4, 4;
          
          STATICTEXT IDC_STATIC { NAME IDS_STATIC22; CENTER_V; ALIGN_LEFT; }
          COMBOBOX IDC_COMBO_MAP_IMPORT
          {
            ALIGN_TOP; SCALE_H; SIZE 150, 0; 
            CHILDS
            {
              0, STR_IMP_NRM; 
              1, STR_IMP_BUMP; 
            }
          }
        }
        CHECKBOX IDC_CHK_IMP_UV { NAME IDS_CHK_IMP_UV; ALIGN_TOP; ALIGN_LEFT;  }
        GROUP IDC_STATIC
        {
          NAME IDS_STATIC8; ALIGN_TOP; SCALE_H; 
          BORDERSIZE 0, 0, 0, 0; 
          COLUMNS 2;
          EQUAL_COLS; 
          SPACE 4, 4;
          
          CHECKBOX IDC_CHK_REPLACE { NAME IDS_CHK_REPLACE; ALIGN_TOP; SCALE_H;  }
          CHECKBOX IDC_CHK_PROMPT { NAME IDS_CHK_PROMPT; ALIGN_TOP; SCALE_H;  }
        }
        BUTTON IDC_BTN_IMPORT { NAME IDS_BTN_IMPORT; ALIGN_TOP; SCALE_H; }
      }
    }
    GROUP IDS_STATIC14
    {
      NAME IDS_STATIC14; ALIGN_TOP; SCALE_H; 
      BORDERSIZE 0, 0, 0, 0; 
      COLUMNS 1;
      SPACE 4, 4;
      
      GROUP IDC_STATIC
      {
        NAME IDS_STATIC23; ALIGN_TOP; SCALE_H; 
        BORDERSTYLE BORDER_GROUP_IN; BORDERSIZE 4, 4, 4, 4; 
        COLUMNS 1;
        SPACE 4, 4;
        
        GROUP IDC_STATIC
        {
          NAME IDS_STATIC18; ALIGN_TOP; SCALE_H; 
          BORDERSIZE 0, 0, 0, 0; 
          COLUMNS 2;
          SPACE 4, 4;
          
          STATICTEXT IDC_STATIC { NAME IDS_STATIC17; CENTER_V; ALIGN_LEFT; }
          FILENAME IDC_COAT_EXE_PATH
          {
            ALIGN_TOP; SCALE_H; 
          }
        }
        CHECKBOX IDC_CHK_COAT_START { NAME IDS_CHK_COAT_START; ALIGN_TOP; ALIGN_LEFT;  }
      }
    }
  }
}