/*
 * ============================================================================
 *  KBC — HTML Template SSR Driven in C
 *  ============================================================================
 */

#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#define MAX_POST 8192
#define MAX_HTML 65536
#define TEMPLATE_PATH "C:\\xampp\\htdocs\\template.html"

static const int PRIZES[15] = {1000,   2000,    3000,    5000,    10000,
                               20000,  40000,   80000,   160000,  320000,
                               640000, 1250000, 2500000, 5000000, 10000000};
#define SAFE_HAVEN_1 4
#define SAFE_HAVEN_2 9

typedef struct {
  char action[64];
  char player_name[64];
  int level;
  int winnings;
  int ll_50;
  int ll_aud;
  int ll_flip;

  char q_text[1024];
  char opt_a[512];
  char opt_b[512];
  char opt_c[512];
  char opt_d[512];
  char correct[8];
  char explain[1024];

  char chosen[8];
  char lifeline[32];

  int hide_a, hide_b, hide_c, hide_d;
  int poll_a, poll_b, poll_c, poll_d;
} FormState;

int get_safe_haven(int level_idx);

/* ══════════════════════════════════════════════════════════════════════════
 *  URL Decoding & HTML Helpers
 * ══════════════════════════════════════════════════════════════════════════ */
void urldecode(char *dst, const char *src) {
  char a, b;
  while (*src) {
    if ((*src == '%') && ((a = src[1]) && (b = src[2])) &&
        (isxdigit(a) && isxdigit(b))) {
      if (a >= 'a')
        a -= 'a' - 'A';
      if (a >= 'A')
        a -= ('A' - 10);
      else
        a -= '0';
      if (b >= 'a')
        b -= 'a' - 'A';
      if (b >= 'A')
        b -= ('A' - 10);
      else
        b -= '0';
      *dst++ = 16 * a + b;
      src += 3;
    } else if (*src == '+') {
      *dst++ = ' ';
      src++;
    } else {
      *dst++ = *src++;
    }
  }
  *dst++ = '\0';
}

void extract_param(const char *body, const char *key, char *out, int out_size) {
  char search[128];
  snprintf(search, sizeof(search), "%s=", key);
  const char *p = strstr(body, search);
  if (!p)
    return;
  if (p != body && *(p - 1) != '&') {
    const char *t = body;
    while ((p = strstr(t, search))) {
      if (p == body || *(p - 1) == '&')
        break;
      t = p + 1;
    }
    if (!p)
      return;
  }
  p += strlen(search);
  int i = 0;
  char raw[MAX_POST];
  while (*p && *p != '&' && i < sizeof(raw) - 1) {
    raw[i++] = *p++;
  }
  raw[i] = '\0';
  urldecode(out, raw);
}
int extract_int(const char *body, const char *key) {
  char buf[32] = "";
  extract_param(body, key, buf, sizeof(buf));
  return atoi(buf);
}

void parse_form_data(const char *body, FormState *s) {
  memset(s, 0, sizeof(FormState));
  s->level = 1;
  extract_param(body, "action", s->action, sizeof(s->action));
  extract_param(body, "player_name", s->player_name, sizeof(s->player_name));

  if (strstr(body, "level="))
    s->level = extract_int(body, "level");
  if (strstr(body, "winnings="))
    s->winnings = extract_int(body, "winnings");
  if (strstr(body, "ll_50="))
    s->ll_50 = extract_int(body, "ll_50");
  if (strstr(body, "ll_aud="))
    s->ll_aud = extract_int(body, "ll_aud");
  if (strstr(body, "ll_flip="))
    s->ll_flip = extract_int(body, "ll_flip");

  extract_param(body, "q_text", s->q_text, sizeof(s->q_text));
  extract_param(body, "opt_a", s->opt_a, sizeof(s->opt_a));
  extract_param(body, "opt_b", s->opt_b, sizeof(s->opt_b));
  extract_param(body, "opt_c", s->opt_c, sizeof(s->opt_c));
  extract_param(body, "opt_d", s->opt_d, sizeof(s->opt_d));
  extract_param(body, "correct", s->correct, sizeof(s->correct));
  extract_param(body, "explain", s->explain, sizeof(s->explain));
  extract_param(body, "chosen", s->chosen, sizeof(s->chosen));
  extract_param(body, "lifeline", s->lifeline, sizeof(s->lifeline));

  s->hide_a = extract_int(body, "hide_a");
  s->hide_b = extract_int(body, "hide_b");
  s->hide_c = extract_int(body, "hide_c");
  s->hide_d = extract_int(body, "hide_d");
  s->poll_a = extract_int(body, "poll_a");
  s->poll_b = extract_int(body, "poll_b");
  s->poll_c = extract_int(body, "poll_c");
  s->poll_d = extract_int(body, "poll_d");
}

/* ══════════════════════════════════════════════════════════════════════════
 *  Templating Engine
 * ══════════════════════════════════════════════════════════════════════════ */
void replace_all(char *str, const char *oldW, const char *newW) {
  char *pos, temp[MAX_HTML];
  int owlen = strlen(oldW);
  if (owlen == 0 || !newW)
    return;
  while ((pos = strstr(str, oldW)) != NULL) {
    int index = pos - str;
    memcpy(temp, str, index);
    temp[index] = '\0';
    strcat(temp, newW);
    strcat(temp, pos + owlen);
    strcpy(str, temp);
  }
}

void extract_section(const char *html, const char *sec_name, char *out) {
  char start_tag[64], end_tag[64];
  sprintf(start_tag, "<!-- SECTION: %s -->", sec_name);
  sprintf(end_tag, "<!-- END_SECTION: %s -->", sec_name);
  const char *start = strstr(html, start_tag);
  if (!start) {
    out[0] = '\0';
    return;
  }
  start += strlen(start_tag);
  const char *end = strstr(start, end_tag);
  if (!end) {
    out[0] = '\0';
    return;
  }
  int len = end - start;
  strncpy(out, start, len);
  out[len] = '\0';
}

void escape_html(const char *src, char *dst) {
  int i = 0;
  while (*src) {
    if (*src == '"') {
      strcpy(dst + i, "&quot;");
      i += 6;
    } else if (*src == '\'') {
      strcpy(dst + i, "&#39;");
      i += 5;
    } else {
      dst[i++] = *src;
    }
    src++;
  }
  dst[i] = '\0';
}

void inject_state(char *html, const FormState *s) {
  char q_esc[2048], ea[1024], eb[1024], ec[1024], ed[1024], exp[2048], nm[128];
  escape_html(s->q_text, q_esc);
  escape_html(s->opt_a, ea);
  escape_html(s->opt_b, eb);
  escape_html(s->opt_c, ec);
  escape_html(s->opt_d, ed);
  escape_html(s->explain, exp);
  escape_html(s->player_name, nm);

  char state_html[4096];
  sprintf(state_html,
          "<input type='hidden' name='player_name' value='%s'>\n"
          "<input type='hidden' name='level' value='%d'>\n"
          "<input type='hidden' name='winnings' value='%d'>\n"
          "<input type='hidden' name='ll_50' value='%d'>\n"
          "<input type='hidden' name='ll_aud' value='%d'>\n"
          "<input type='hidden' name='ll_flip' value='%d'>\n"
          "<input type='hidden' name='q_text' value='%s'>\n"
          "<input type='hidden' name='opt_a' value='%s'>\n"
          "<input type='hidden' name='opt_b' value='%s'>\n"
          "<input type='hidden' name='opt_c' value='%s'>\n"
          "<input type='hidden' name='opt_d' value='%s'>\n"
          "<input type='hidden' name='correct' value='%s'>\n"
          "<input type='hidden' name='explain' value='%s'>\n"
          "<input type='hidden' name='hide_a' value='%d'>\n"
          "<input type='hidden' name='hide_b' value='%d'>\n"
          "<input type='hidden' name='hide_c' value='%d'>\n"
          "<input type='hidden' name='hide_d' value='%d'>\n"
          "<input type='hidden' name='poll_a' value='%d'>\n"
          "<input type='hidden' name='poll_b' value='%d'>\n"
          "<input type='hidden' name='poll_c' value='%d'>\n"
          "<input type='hidden' name='poll_d' value='%d'>\n",
          nm, s->level, s->winnings, s->ll_50, s->ll_aud, s->ll_flip, q_esc, ea,
          eb, ec, ed, s->correct, exp, s->hide_a, s->hide_b, s->hide_c,
          s->hide_d, s->poll_a, s->poll_b, s->poll_c, s->poll_d);
  replace_all(html, "{{HIDDEN_STATE}}", state_html);
}

/* ══════════════════════════════════════════════════════════════════════════
 *  Game Screens
 * ══════════════════════════════════════════════════════════════════════════ */
void render_screen(const char *sec_name, FormState *s) {
  FILE *f = fopen(TEMPLATE_PATH, "r");
  if (!f) {
    printf(
        "Content-Type: text/plain\r\n\r\nError: Cannot open template.html\n");
    exit(1);
  }
  char tpl[MAX_HTML];
  size_t bytes = fread(tpl, 1, MAX_HTML - 1, f);
  tpl[bytes] = '\0';
  fclose(f);

  char section[MAX_HTML];
  extract_section(tpl, sec_name, section);

  // Base Replacements
  replace_all(tpl, "{{TITLE}}", "KBC Game");
  replace_all(tpl, "{{MAIN_CONTENT}}", section);

  // Inject Hidden Form States
  inject_state(tpl, s);

  // Common Base Values
  char buf[64];
  replace_all(tpl, "{{NAME}}", s->player_name);
  sprintf(buf, "%d", s->winnings);
  replace_all(tpl, "{{WINNINGS}}", buf);

  // Specific Screen Replacements
  if (strcmp(sec_name, "FETCH") == 0) {
    const char *diff = (s->level <= 5)    ? "easy"
                       : (s->level <= 10) ? "medium"
                                          : "hard";
    replace_all(tpl, "{{DIFF}}", diff);
    replace_all(tpl, "{{CAT}}", (rand() % 2 == 0) ? "18" : "17");
  } else if (strcmp(sec_name, "BOARD") == 0) {
    replace_all(tpl, "{{Q_TEXT}}", s->q_text);
    replace_all(tpl, "{{A_TEXT}}", s->opt_a);
    replace_all(tpl, "{{B_TEXT}}", s->opt_b);
    replace_all(tpl, "{{C_TEXT}}", s->opt_c);
    replace_all(tpl, "{{D_TEXT}}", s->opt_d);
    replace_all(tpl, "{{D_50}}", s->ll_50 ? "disabled" : "");
    replace_all(tpl, "{{D_AUD}}", s->ll_aud ? "disabled" : "");
    replace_all(tpl, "{{D_FLIP}}", s->ll_flip ? "disabled" : "");
    replace_all(tpl, "{{HIDE_A}}", s->hide_a ? "hidden-opt" : "");
    replace_all(tpl, "{{HIDE_B}}", s->hide_b ? "hidden-opt" : "");
    replace_all(tpl, "{{HIDE_C}}", s->hide_c ? "hidden-opt" : "");
    replace_all(tpl, "{{HIDE_D}}", s->hide_d ? "hidden-opt" : "");

    char poll_html[2048] = "";
    if (s->poll_a || s->poll_b || s->poll_c || s->poll_d) {
      sprintf(
          poll_html,
          "<h3 class='gold' style='text-align:center;'>Audience Says:</h3>"
          "<div class='poll-container'>"
          "<div style='text-align:center;'><div class='gold'>%d%%</div><div "
          "class='poll-bar' style='height:%dpx'></div><div>A</div></div>"
          "<div style='text-align:center;'><div class='gold'>%d%%</div><div "
          "class='poll-bar' style='height:%dpx'></div><div>B</div></div>"
          "<div style='text-align:center;'><div class='gold'>%d%%</div><div "
          "class='poll-bar' style='height:%dpx'></div><div>C</div></div>"
          "<div style='text-align:center;'><div class='gold'>%d%%</div><div "
          "class='poll-bar' style='height:%dpx'></div><div>D</div></div>"
          "</div>",
          s->poll_a, s->poll_a, s->poll_b, s->poll_b, s->poll_c, s->poll_c,
          s->poll_d, s->poll_d);
    }
    replace_all(tpl, "{{AUDIENCE_POLL}}", poll_html);

    char ladder_html[2048] = "";
    for (int i = 0; i < 15; i++) {
      const char *cls = "ladder-item";
      if (i < s->level - 1)
        cls = "ladder-item passed";
      else if (i == s->level - 1)
        cls = "ladder-item current";
      int safe = (i == SAFE_HAVEN_1 || i == SAFE_HAVEN_2);
      char l_item[256];
      sprintf(
          l_item,
          "<div class='%s %s'><span>%d</span><span>&#8377;%d %s</span></div>",
          cls, safe ? "safe" : "", i + 1, PRIZES[i], safe ? "&#9733;" : "");
      strcat(ladder_html, l_item);
    }
    replace_all(tpl, "{{LADDER}}", ladder_html);
  } else if (strcmp(sec_name, "RESULT") == 0) {
    replace_all(tpl, "{{EXPLAIN}}", s->explain);

    int safe1 = get_safe_haven(s->level - 2);
    char header[512] = "", btns[1024] = "";

    if (strcmp(s->chosen, s->correct) == 0) {
      s->winnings = PRIZES[s->level - 1];
      sprintf(header,
              "<h1 style='color:#4caf50;'>CORRECT!</h1><h2>You won <span "
              "class='gold'>&#8377;%d</span></h2>",
              s->winnings);

      if (s->level == 15) {
        strcat(header,
               "<h1 class='gold' style='font-size:3em;'>CROREPATI!</h1>");
        sprintf(btns, "<a href='/cgi-bin/kbc.cgi' class='btn' "
                      "style='text-decoration:none;'>Play Again</a>");
      } else {
        s->level += 1;
        sprintf(
            btns,
            "<button type='submit' name='action' value='fetch_question' "
            "class='btn btn-green'>PLAY NEXT QUESTION</button>"
            "<button type='submit' name='action' value='quit' class='btn "
            "btn-red' style='margin-left:20px;'>QUIT (Keep &#8377;%d)</button>",
            s->winnings);
      }
    } else if (strcmp(s->chosen, "QUIT") == 0) {
      sprintf(header,
              "<h1 class='gold'>YOU WALKED AWAY</h1><h2>Final Prize: <span "
              "style='color:#4caf50;'>&#8377;%d</span></h2>",
              s->winnings);
      sprintf(btns, "<a href='/cgi-bin/kbc.cgi' class='btn' "
                    "style='text-decoration:none;'>Play Again</a>");
    } else {
      int final_prize = get_safe_haven(s->level - 1);
      sprintf(header,
              "<h1 style='color:#f44336;'>WRONG!</h1>"
              "<h2>Correct answer: <span class='gold'>%s</span>.</h2>"
              "<h3>You drop to safe haven: <span "
              "class='gold'>&#8377;%d</span></h3>",
              s->correct, final_prize);
      sprintf(btns, "<a href='/cgi-bin/kbc.cgi' class='btn' "
                    "style='text-decoration:none;'>Play Again</a>");
    }

    replace_all(tpl, "{{RESULT_HEADER}}", header);
    replace_all(tpl, "{{NEXT_BUTTONS}}", btns);

    // Re-inject updated state for the 'next phase'
    replace_all(tpl, "{{HIDDEN_STATE_NEXT}}", "{{HIDDEN_STATE}}");
    inject_state(tpl, s);
  }

  // Clean up any remaining block comments or unfilled vars
  replace_all(tpl, "{{HIDDEN_STATE}}", "");
  replace_all(tpl, "{{HIDDEN_STATE_NEXT}}", "");

  // Cut off the template bank at the bottom
  char *end_html = strstr(tpl, "</html>");
  if (end_html)
    end_html[7] = '\0';

  printf("Content-Type: text/html\r\n\r\n");
  printf("%s", tpl);
}

int get_safe_haven(int level_idx) {
  if (level_idx > SAFE_HAVEN_2)
    return PRIZES[SAFE_HAVEN_2];
  if (level_idx > SAFE_HAVEN_1)
    return PRIZES[SAFE_HAVEN_1];
  return 0;
}

/* ══════════════════════════════════════════════════════════════════════════
 *  MAIN LOGIC
 * ══════════════════════════════════════════════════════════════════════════ */
int main(void) {
  srand((unsigned int)time(NULL));

  const char *method = getenv("REQUEST_METHOD");
  if (!method || strcmp(method, "POST") != 0) {
    FormState s;
    memset(&s, 0, sizeof(s));
    s.level = 1;
    render_screen("START", &s);
    return 0;
  }

  char body[MAX_POST] = "";
  int content_length = 0;
  const char *cl = getenv("CONTENT_LENGTH");
  if (cl)
    content_length = atoi(cl);
  if (content_length > 0 && content_length < sizeof(body)) {
    fread(body, 1, content_length, stdin);
  }

  FormState s;
  parse_form_data(body, &s);

  if (strcmp(s.action, "fetch_question") == 0) {
    // Reset round-specific visual states for the new question
    s.hide_a = s.hide_b = s.hide_c = s.hide_d = 0;
    s.poll_a = s.poll_b = s.poll_c = s.poll_d = 0;

    render_screen("FETCH",
                  &s); // Uses JS to fetch The Trivia API 100% of the time
  } else if (strcmp(s.action, "show_question") == 0) {
    render_screen("BOARD", &s);
  } else if (strcmp(s.action, "answer") == 0) {
    render_screen("RESULT", &s);
  } else if (strcmp(s.action, "lifeline") == 0) {
    if (strcmp(s.lifeline, "flip") == 0 && !s.ll_flip) {
      s.ll_flip = 1;
      render_screen("FETCH", &s);
    } else if (strcmp(s.lifeline, "50") == 0 && !s.ll_50) {
      s.ll_50 = 1;
      int h[4] = {0, 0, 0, 0};
      int removed = 0;
      char target = s.correct[0];
      while (removed < 2) {
        int r = rand() % 4;
        if (r != (target - 'A') && !h[r]) {
          h[r] = 1;
          removed++;
        }
      }
      s.hide_a = h[0];
      s.hide_b = h[1];
      s.hide_c = h[2];
      s.hide_d = h[3];
      render_screen("BOARD", &s);
    } else if (strcmp(s.lifeline, "aud") == 0 && !s.ll_aud) {
      s.ll_aud = 1;
      int cIdx = s.correct[0] - 'A';
      int p[4] = {0, 0, 0, 0};
      p[cIdx] = 50 + (rand() % 21);
      int rem = 100 - p[cIdx];
      for (int i = 0; i < 4; i++) {
        if (i == cIdx)
          continue;
        int share = rand() % (rem / 2 + 2);
        p[i] = share;
        rem -= share;
      }
      for (int i = 3; i >= 0; i--) {
        if (i != cIdx) {
          p[i] += rem;
          break;
        }
      }
      s.poll_a = p[0];
      s.poll_b = p[1];
      s.poll_c = p[2];
      s.poll_d = p[3];
      render_screen("BOARD", &s);
    } else {
      render_screen("BOARD", &s);
    }
  } else if (strcmp(s.action, "quit") == 0) {
    strcpy(s.chosen, "QUIT");
    render_screen("RESULT", &s);
  } else {
    render_screen("START", &s);
  }
  return 0;
}
