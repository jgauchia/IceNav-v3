/**
 * @file wpt.h
 * @author Jordi Gauchía (jgauchia@jgauchia.com)
 * @brief  Waypoints functions and routines
 * @version 0.1
 * @date 2022-10-12
 */

typedef struct
{
    int number;           // Field 1
    String name;          // Field 2
    float latitude;       // Field 3
    float longitude;      // FIeld 4
    String description;   // Field 11
    float altitude;       // Field 15
} oziexplorer_struct;

#define MAX_WPT 150
String wpt_line;
oziexplorer_struct WPT[MAX_WPT];

/**
 * @brief Read and parse OziExplorer Waypoint file and stores lines in a string array, returns number of waypoints
 *
 * @param filename -> WPT file
 * @return int -> Number of waypoints
 */
int read_wpt_file(const char *filename)
{
    int total_wpt = 0;
    char byte;
    char s_buf[150];
    char *s_parse;
    int parse_field = 1;

    File wptFS;
    wptFS = SD.open(filename);

    if (!wptFS)
    {
        debug->println("File not found");
        return 0;
    }

    while (wptFS.available())
    {
        byte = wptFS.read();
        if (byte != '\n')
            wpt_line += byte;
        else
        {
            /* OziExplorer first 4 lines of file header are not válid , discarded */
            if (total_wpt > 3)
            {
                wpt_line.toCharArray(s_buf, sizeof(s_buf));
                debug->println(s_buf);
                s_parse = strtok(s_buf, ",");
                while (s_parse != NULL)
                {
                    switch (parse_field)
                    {
                    case 1:
                        WPT[total_wpt-4].number = atoi(s_parse);
                        break;
                    case 2:
                        WPT[total_wpt-4].name = s_parse;
                        break;
                    case 3:
                        WPT[total_wpt-4].latitude = atof(s_parse);
                        break;
                    case 4:
                        WPT[total_wpt-4].longitude = atof(s_parse);
                        break;
                    case 11:
                        WPT[total_wpt-4].description = s_parse;
                        break;
                    case 15:
                        WPT[total_wpt-4].altitude = atoi(s_parse) * 0.3048;
                        break;
                    }
                    parse_field++;
                    s_parse = strtok(NULL, ",");
                }
                parse_field = 1;
                s_buf[0] = '\0';
            }
            total_wpt++;
            wpt_line.clear();
        }
    }
    wptFS.close();
    return total_wpt - 5;
}