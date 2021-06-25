import numpy as np


class KalmanFilter:
    def __init__(self, x0, p0, r=1, q=np.array([0, 2, 2 ** 11, 0]), ts=1):
        # Constants
        self.Ts = ts
        self.R = r
        self.Q = q

        # State: Phase, Freq., Amp.
        self.x = x0
        self.p = p0

    def binsin(self, phi):
        return np.sin(np.pi * 2 ** (-17) * phi)

    def bincos(self, phi):
        return np.cos(np.pi * phi * 2 ** (-17))

    def predict_state(self, state):
        return np.array([state[0] + self.Ts * state[1], state[1], state[2]])

    def predict_P(self, P):
        P_0 = P[0] + 2 * self.Ts * P[3] + self.Ts ** 2 * P[1] + self.Q[0]
        P_1 = P[1] + self.Q[1]
        P_2 = P[2] + self.Q[2]
        P_od = P[3] + self.Ts * P[1] + self.Q[3]
        return np.array([P_0, P_1, P_2, P_od])

    def alpha(self, state, P):
        a = ((np.pi * 2 ** (-15)) ** 2 * P[0] * (state[2] ** 2) * (self.bincos(state[0])) ** 2 + P[2] * (
            self.binsin(state[0])) ** 2)
        return a

    def gain(self, state, P):
        s = (self.alpha(state, P) + self.R)
        K_0 = P[0] * state[2] * np.pi * 2 ** (-15) * self.bincos(state[0])
        K_1 = P[3] * state[2] * np.pi * 2 ** (-15) * self.bincos(state[0])
        K_2 = P[2] * self.binsin(state[0])
        return np.array([K_0, K_1, K_2]) / s

    def update_state(self, state, P, measurement):
        s = (self.alpha(state, P) + self.R)
        y = measurement - state[2] * self.binsin(state[0])
        return (state + y * self.gain(state, P)), y * self.gain(state, P)

    def update_P(self, state, P):
        a = self.alpha(state, P).astype(int)
        return np.floor((1 - a / (a + self.R)) * P), (1 - a / (a + self.R))

    def filter(self, measurement):
        # # Predict
        # self.x = np.dot(self.f, self.x)
        # self.p = self.f.dot(self.p.dot(self.f.T)) + self.Q
        #
        # # Update
        # h = -self.x[2] * sin(self.x[0])
        # y = measurement - h.dot(self.x)
        # s = h * self.p * h.T + self.R
        # k = self.p * h.t * np.linalg.inv(s)
        # self.x = self.x + k * y
        # self.p = (np.eye([3, 3]) - k*h).dot(self.p)
        # return measurement - h*self.x

        # prediction:
        X_tilde = self.predict_state(self.x)
        P_tilde = self.predict_P(self.p)

        # store prev phase
        ph_prev = self.x[0] % np.pi

        # update:
        self.x, y = self.update_state(X_tilde, P_tilde, measurement)
        self.p, _ = self.update_P(X_tilde, P_tilde)

        cross = 1 if np.abs(ph_prev - (self.x[0] % 2 ** 18)) > 2 ** 17 else 0
        return y, self.x, cross
