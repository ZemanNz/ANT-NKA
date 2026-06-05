#!/usr/bin/env python3
"""
Simulace hřiště Roadside Assistance 2026 — přesně dle soutěžní logiky v main.cpp
Robot jezdí POUZE v ose X. Otáčí se k bateriím, rameno sahá na bok k dockům.
Ovládání: 1-4 = kombinace, SPACE = start, R = reset, T = tým, ESC = konec
"""
import pygame, sys, math

# === ROZMĚRY (mm) ===
FIELD_W, FIELD_H = 3000.0, 2000.0
START_ZONE = 500.0
ZADEK_OD_STREDU = 80.0
RAMENO_OD_STREDU = 40.0

# Docky
DOCK_FIRST_EDGE = 750.0
DOCK_W = 120.0
DOCK_PITCH = DOCK_W + 80.0  # gap=80
DOCK_FC = DOCK_FIRST_EDGE + DOCK_W / 2.0
DOCK_X = [DOCK_FC + i * DOCK_PITCH for i in range(8)]
DOCK_DEPTH = 180.0

# Baterie
BAT_COLS = [1350.0, 1480.0, 1620.0, 1750.0]
BAT_Y_CLOSE, BAT_Y_FAR = 850.0, 950.0
BAT_POS = []
for c in BAT_COLS:
    BAT_POS.append((c, BAT_Y_CLOSE))
    BAT_POS.append((c, BAT_Y_FAR))

# Kombinace (pohled Blue)
LAYOUTS = [
    ['B','R','R','R','B','B','B','R'],
    ['R','B','R','R','B','B','R','B'],
    ['R','R','B','R','B','R','B','B'],
    ['R','R','R','B','R','B','B','B'],
]

# === PYGAME ===
SCALE = 0.38
MARGIN = 50
UI_H = 80
W = int(FIELD_W * SCALE) + 2 * MARGIN
H = int(FIELD_H * SCALE) + 2 * MARGIN + UI_H

BG = (25, 25, 35); WHITE = (240, 240, 240); RED = (220, 55, 55); BLUE = (55, 100, 220)
PURPLE = (150, 60, 200); YELLOW = (240, 200, 40); GRAY = (120, 120, 130)
DKGRAY = (60, 60, 70); ROAD = (55, 55, 60); ORANGE = (255, 160, 30); GREEN = (50, 190, 80)

def mm2px(x, y):
    return int(MARGIN + x * SCALE), int(MARGIN + (FIELD_H - y) * SCALE)
def mm2sz(w, h):
    return max(1, int(w * SCALE)), max(1, int(h * SCALE))

ROBOT_Y = 500.0  # Pevná Y dráha robota

# =============================================
# PLÁNOVÁNÍ TRASY — přesně dle main.cpp
# =============================================
def plan_race(layout, is_red):
    """Vrací list kroků přesně dle soutěžní logiky v main.cpp."""
    my_color = 'R' if is_red else 'B'
    start_x = (FIELD_W - START_ZONE/2 - ZADEK_OD_STREDU) if is_red else (START_ZONE/2 + ZADEK_OD_STREDU)

    # Najdi naše docky
    my_docks = [i for i in range(8) if layout[i] == my_color]
    # Seřaď od nejvzdálenějšího od startu
    my_docks.sort(key=lambda i: -abs(DOCK_X[i] - start_x))

    # Sloupce baterií seřazené od nejvzdálenějšího
    cols = sorted(BAT_COLS, key=lambda c: -abs(c - start_x))

    steps = []
    pos_x = start_x

    for cycle in range(min(4, len(my_docks))):
        bat_x = cols[cycle]
        dock_idx = my_docks[cycle]
        dock_x = DOCK_X[dock_idx]

        # 1. Dojeď k baterii (arm target)
        if is_red:
            center_bat = bat_x - RAMENO_OD_STREDU
        else:
            center_bat = bat_x + RAMENO_OD_STREDU
        steps.append(("DRIVE", center_bat))
        steps.append(("WAIT", 0.2))

        # 2. Otoč se k bateriím
        grab_angle = -90.0 if is_red else 90.0
        steps.append(("TURN", grab_angle))
        steps.append(("WAIT", 0.15))

        # 3. Rameno: Center → Down → Magnet → Up
        steps.append(("ARM_GRAB",))
        steps.append(("WAIT", 0.6))
        steps.append(("GRAB_BAT", cycle))  # cycle = index sloupce

        # 4. Otoč zpět
        steps.append(("TURN", 0.0))
        steps.append(("WAIT", 0.15))
        steps.append(("ARM_CENTER",))
        steps.append(("WAIT", 0.2))

        # 5. Dojeď k docku
        if is_red:
            center_dock = dock_x - RAMENO_OD_STREDU
        else:
            center_dock = dock_x + RAMENO_OD_STREDU
        steps.append(("DRIVE", center_dock))
        steps.append(("WAIT", 0.2))

        # 6. Rameno na bok k docku → dolů → pusť
        steps.append(("ARM_DROP",))
        steps.append(("WAIT", 0.6))
        steps.append(("DROP_DOCK", dock_idx))
        steps.append(("WAIT", 0.3))
        steps.append(("ARM_CENTER",))
        steps.append(("WAIT", 0.2))

    # Návrat domů — couvání
    home_x = (FIELD_W - 50.0) if is_red else 50.0
    steps.append(("DRIVE_BACK", home_x))
    steps.append(("WAIT", 0.3))
    steps.append(("DONE",))
    return steps

# =============================================
# ROBOT
# =============================================
class Robot:
    def __init__(self):
        self.is_red = True
        self.reset()

    def reset(self):
        self.x = (FIELD_W - START_ZONE/2 - ZADEK_OD_STREDU) if self.is_red else (START_ZONE/2 + ZADEK_OD_STREDU)
        self.y = ROBOT_Y
        self.angle = 0.0
        self.arm = "center"  # center / grab_down / drop_side
        self.has_battery = False
        self.state = "IDLE"
        self.plan = []
        self.target_x = self.x
        self.target_angle = 0.0
        self.wait_t = 0
        self.delivered = 0
        self.heading = 0
        self.phase_text = "Připraven"
        self.backing = False

    def start(self, layout):
        self.reset()
        self.plan = plan_race(layout, self.is_red)
        self.state = "IDLE"

    def update(self, dt, bat_st, dock_st):
        if self.state == "IDLE":
            if self.plan:
                self._next(bat_st, dock_st)
            return
        if self.state == "DRIVING":
            dx = self.target_x - self.x
            spd = 600.0 if not self.backing else 400.0
            step = spd * dt
            if abs(dx) <= step:
                self.x = self.target_x
                self.heading = 0
                self.state = "IDLE"
            else:
                d = 1 if dx > 0 else -1
                self.x += d * step
                self.heading = d
        elif self.state == "TURNING":
            diff = self.target_angle - self.angle
            if diff > 180: diff -= 360
            if diff < -180: diff += 360
            step = 200.0 * dt
            if abs(diff) <= step:
                self.angle = self.target_angle
                self.state = "IDLE"
            else:
                self.angle += step * (1 if diff > 0 else -1)
        elif self.state == "WAITING":
            self.wait_t -= dt
            if self.wait_t <= 0:
                self.state = "IDLE"

    def _next(self, bat_st, dock_st):
        if not self.plan:
            self.state = "DONE"; return
        cmd = self.plan.pop(0)
        op = cmd[0]
        if op == "DRIVE":
            self.target_x = cmd[1]; self.state = "DRIVING"; self.backing = False
            self.phase_text = "Jede k cíli..."
        elif op == "DRIVE_BACK":
            self.target_x = cmd[1]; self.state = "DRIVING"; self.backing = True
            self.phase_text = "Couvá domů..."
        elif op == "TURN":
            self.target_angle = cmd[1]; self.state = "TURNING"
            self.phase_text = f"Otáčí se na {cmd[1]:.0f}°"
        elif op == "ARM_GRAB":
            self.arm = "grab_down"
            self.phase_text = "Rameno dolů — sbírá baterku"
            self.state = "IDLE"
        elif op == "ARM_DROP":
            self.arm = "drop_side"
            self.phase_text = "Rameno na bok — vkládá do docku"
            self.state = "IDLE"
        elif op == "ARM_CENTER":
            self.arm = "center"
            self.state = "IDLE"
        elif op == "GRAB_BAT":
            col_idx = cmd[1]
            # Seber bližší baterii z daného sloupce
            for i, (bx, by) in enumerate(BAT_POS):
                if bat_st[i] and abs(bx - BAT_COLS[col_idx]) < 1:
                    bat_st[i] = False; break
            self.has_battery = True
            self.delivered  # jen inkrementujeme při dropu
            self.phase_text = "Baterka chycena!"
            self.state = "IDLE"
        elif op == "DROP_DOCK":
            dock_st[cmd[1]] = True
            self.has_battery = False
            self.delivered += 1
            self.phase_text = f"Baterka vložena do docku {cmd[1]+1}!"
            self.state = "IDLE"
        elif op == "WAIT":
            self.wait_t = cmd[1]; self.state = "WAITING"
        elif op == "DONE":
            self.state = "DONE"; self.heading = 0
            self.phase_text = f"Hotovo! Doručeno: {self.delivered}/4"

# =============================================
# VYKRESLENÍ
# =============================================
def draw_field(s):
    fx, fy = mm2px(0, FIELD_H); fw, fh = mm2sz(FIELD_W, FIELD_H)
    pygame.draw.rect(s, WHITE, (fx, fy, fw, fh))
    pygame.draw.rect(s, DKGRAY, (fx, fy, fw, fh), 4)
    ry1, ry2 = 600.0, 1400.0
    rx, ry = mm2px(0, ry2); rw, rh = mm2sz(FIELD_W, ry2-ry1)
    pygame.draw.rect(s, ROAD, (rx, ry, rw, rh))
    for i in range(0, 3000, 200):
        pygame.draw.line(s, WHITE, mm2px(i, 1000), mm2px(i+100, 1000), 2)

def draw_zones(s):
    zw, zh = mm2sz(START_ZONE, START_ZONE)
    font = pygame.font.SysFont("monospace", 13, bold=True)
    # Červená VPRAVO
    rx, ry = mm2px(FIELD_W-START_ZONE, START_ZONE)
    sf = pygame.Surface((zw, zh), pygame.SRCALPHA); sf.fill((220,55,55,60))
    s.blit(sf, (rx, ry)); pygame.draw.rect(s, RED, (rx,ry,zw,zh), 2)
    t = font.render("RED", True, RED); s.blit(t, (rx+zw//2-t.get_width()//2, ry+zh//2-6))
    # Modrá VLEVO
    bx, by = mm2px(0, START_ZONE)
    sf2 = pygame.Surface((zw, zh), pygame.SRCALPHA); sf2.fill((55,100,220,60))
    s.blit(sf2, (bx, by)); pygame.draw.rect(s, BLUE, (bx,by,zw,zh), 2)
    t2 = font.render("BLUE", True, BLUE); s.blit(t2, (bx+zw//2-t2.get_width()//2, by+zh//2-6))

def draw_docks(s, layout, dock_st):
    f = pygame.font.SysFont("monospace", 12, bold=True)
    for i in range(8):
        c = RED if layout[i]=='R' else BLUE
        cx, cy = mm2px(DOCK_X[i], 0); dw, dh = mm2sz(DOCK_W, DOCK_DEPTH)
        x, y = cx-dw//2, cy-dh
        if dock_st[i]:
            pygame.draw.rect(s, c, (x,y,dw,dh))
            bw, bh = mm2sz(50,50)
            pygame.draw.rect(s, PURPLE, (cx-bw//2, cy-dh//2-bh//2, bw, bh))
        else:
            sf = pygame.Surface((dw,dh), pygame.SRCALPHA); sf.fill((*c,50)); s.blit(sf,(x,y))
        pygame.draw.rect(s, c, (x,y,dw,dh), 2)
        t = f.render(str(i+1), True, WHITE); s.blit(t, (cx-t.get_width()//2, cy-dh//2-t.get_height()//2))

def draw_bats(s, bat_st):
    bw, bh = mm2sz(55,55)
    for i,(bx,by) in enumerate(BAT_POS):
        if bat_st[i]:
            px,py = mm2px(bx,by)
            pygame.draw.rect(s, PURPLE, (px-bw//2, py-bh//2, bw, bh))
            pygame.draw.rect(s, (100,30,140), (px-bw//2, py-bh//2, bw, bh), 2)
            pygame.draw.circle(s, GRAY, (px,py), int(16*SCALE), 2)

def draw_robot(s, robot):
    px, py = mm2px(robot.x, robot.y)
    bw, bh = mm2sz(200, 160)

    # Tělo — otočené podle angle
    body = pygame.Surface((bw, bh), pygame.SRCALPHA)
    pygame.draw.rect(body, (70,70,80), (0,0,bw,bh), border_radius=5)
    pygame.draw.rect(body, ORANGE, (0,0,bw,bh), 2, border_radius=5)
    # Šipka směru
    pygame.draw.polygon(body, YELLOW, [(bw//2+8, bh//2), (bw//2-4, bh//2-5), (bw//2-4, bh//2+5)])
    rot = pygame.transform.rotate(body, robot.angle)
    s.blit(rot, (px-rot.get_width()//2, py-rot.get_height()//2))

    # Rameno
    arm_len = int(320 * SCALE)
    if robot.arm == "grab_down":
        # Rameno natažené směrem k bateriím (nahoru na obrazovce = +Y na hřišti)
        # Směr závisí na angle: po otočení -90° (RED) rameno směřuje nahoru
        rad = math.radians(robot.angle + 90)
        ex = px + math.cos(rad) * arm_len
        ey = py - math.sin(rad) * arm_len
        pygame.draw.line(s, ORANGE, (px, py), (int(ex), int(ey)), 4)
        pygame.draw.circle(s, YELLOW, (int(ex), int(ey)), 6)
        if robot.has_battery:
            pygame.draw.rect(s, PURPLE, (int(ex)-8, int(ey)-8, 16, 16))
    elif robot.arm == "drop_side":
        # Rameno natažené k dockům (dolů na obrazovce = -Y na hřišti)
        ex = px
        ey = py + arm_len
        pygame.draw.line(s, ORANGE, (px, py), (ex, ey), 4)
        pygame.draw.circle(s, YELLOW, (ex, ey), 6)
        if robot.has_battery:
            pygame.draw.rect(s, PURPLE, (ex-8, ey-8, 16, 16))
    else:
        pygame.draw.circle(s, ORANGE, (px, py), 5)
        if robot.has_battery:
            pygame.draw.rect(s, PURPLE, (px-8, py-8, 16, 16))
            pygame.draw.rect(s, (100,30,140), (px-8, py-8, 16, 16), 1)

    # Jízdní dráha
    ly = mm2px(0, ROBOT_Y)[1]
    pygame.draw.line(s, (50,50,55), (MARGIN, ly), (MARGIN+int(FIELD_W*SCALE), ly), 1)

def draw_ui(s, layout_idx, robot, is_red):
    y0 = H - UI_H + 5
    f = pygame.font.SysFont("monospace", 17, bold=True)
    fs = pygame.font.SysFont("monospace", 13)

    pygame.draw.rect(s, (40,40,50), (0, H-UI_H, W, UI_H))
    pygame.draw.line(s, ORANGE, (0, H-UI_H), (W, H-UI_H), 2)

    team_c = RED if is_red else BLUE
    team_n = "ČERVENÝ" if is_red else "MODRÝ"
    t = f.render(f"Tým: {team_n}  |  Kombinace: {layout_idx+1}/4", True, WHITE)
    s.blit(t, (15, y0))

    # Mini docky
    for i in range(8):
        c = RED if LAYOUTS[layout_idx][i]=='R' else BLUE
        pygame.draw.rect(s, c, (420+i*22, y0+3, 18, 12))
        pygame.draw.rect(s, WHITE, (420+i*22, y0+3, 18, 12), 1)

    # Fáze
    t2 = fs.render(f"{robot.phase_text}  |  Baterky: {robot.delivered}/4  |  X={robot.x:.0f}mm", True, YELLOW)
    s.blit(t2, (15, y0+24))

    t3 = fs.render("1-4: kombinace  SPACE: start  T: tým  R: reset  ESC: konec", True, GRAY)
    s.blit(t3, (15, y0+44))

    # Stav
    st = {"IDLE":"⏸","DRIVING":"▶","TURNING":"↻","WAITING":"⏳","DONE":"✓"}.get(robot.state,"?")
    t4 = f.render(st, True, GREEN if robot.state=="DONE" else YELLOW)
    s.blit(t4, (W-60, y0))

def draw_legend(s):
    fs = pygame.font.SysFont("monospace", 12)
    items = [(PURPLE,"Baterie"),(RED,"Červený dock"),(BLUE,"Modrý dock"),(ORANGE,"Robot")]
    x0 = MARGIN
    for c, l in items:
        pygame.draw.rect(s, c, (x0, 8, 12, 12))
        t = fs.render(l, True, WHITE); s.blit(t, (x0+16, 6)); x0 += t.get_width() + 30

# =============================================
# MAIN
# =============================================
def main():
    pygame.init()
    screen = pygame.display.set_mode((W, H))
    pygame.display.set_caption("Roadside Assistance 2026 — Soutěžní simulace")
    clock = pygame.time.Clock()

    layout_idx = 0
    robot = Robot()
    bat_st = [True]*len(BAT_POS)
    dock_st = [False]*8
    running = False

    while True:
        dt = clock.tick(60) / 1000.0
        for ev in pygame.event.get():
            if ev.type == pygame.QUIT: pygame.quit(); sys.exit()
            if ev.type == pygame.KEYDOWN:
                if ev.key == pygame.K_ESCAPE: pygame.quit(); sys.exit()
                if ev.key == pygame.K_t:
                    robot.is_red = not robot.is_red; running=False; robot.reset()
                    bat_st=[True]*len(BAT_POS); dock_st=[False]*8
                if ev.key in (pygame.K_1,pygame.K_KP1): layout_idx=0; running=False; robot.reset(); bat_st=[True]*len(BAT_POS); dock_st=[False]*8
                if ev.key in (pygame.K_2,pygame.K_KP2): layout_idx=1; running=False; robot.reset(); bat_st=[True]*len(BAT_POS); dock_st=[False]*8
                if ev.key in (pygame.K_3,pygame.K_KP3): layout_idx=2; running=False; robot.reset(); bat_st=[True]*len(BAT_POS); dock_st=[False]*8
                if ev.key in (pygame.K_4,pygame.K_KP4): layout_idx=3; running=False; robot.reset(); bat_st=[True]*len(BAT_POS); dock_st=[False]*8
                if ev.key == pygame.K_SPACE and not running:
                    running=True; bat_st=[True]*len(BAT_POS); dock_st=[False]*8
                    robot.start(LAYOUTS[layout_idx])
                if ev.key == pygame.K_r:
                    running=False; robot.reset(); bat_st=[True]*len(BAT_POS); dock_st=[False]*8

        if running:
            robot.update(dt, bat_st, dock_st)

        screen.fill(BG)
        draw_field(screen)
        draw_zones(screen)
        draw_docks(screen, LAYOUTS[layout_idx], dock_st)
        draw_bats(screen, bat_st)
        draw_robot(screen, robot)
        draw_legend(screen)
        draw_ui(screen, layout_idx, robot, robot.is_red)
        pygame.display.flip()

if __name__ == "__main__":
    main()
