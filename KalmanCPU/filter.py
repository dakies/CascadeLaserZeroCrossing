import numpy as np


class KalmanFilter:
    def __init__(self, x0=np.array([0, 99291.8, 0.012]), p0=np.array([100, 100, 100, 100]), r=0.000001,
                 q=np.array([0.01, 0.001, 0.001, 0]), ts=4*10**(-6)):
        # Constants
        self.Ts = ts
        self.R = r
        self.Q = q
        # State: Phase, Freq., Amp.
        self.x = x0
        self.p = p0

    # Predict functions:
    def predict_state(self):
        state = self.x
        ts = self.Ts
        self.x = np.array([state[0] + ts * state[1], state[1], state[2]])

    def predict_p(self):
        """ We return fist all diagonal terms and then the off-diagonal element. """
        p = self.p
        ts = self.Ts
        q = self.Q
        p_0 = p[0] + 2 * ts * p[3] + ts ** 2 * p[1] + q[0]
        p_1 = p[1] + q[1]
        p_2 = p[2] + q[2]
        p_od = p[3] + ts * p[1] + q[3]
        self.p = np.array([p_0, p_1, p_2, p_od])

    # Update functions:
    def alpha(self):
        state = self.x
        p = self.p
        return p[0] * (state[2] ** 2) * (np.cos(state[0])) ** 2 + p[2] * (np.sin(state[0])) ** 2

    def gain(self):
        state = self.x
        p = self.p
        r = self.R
        s = self.alpha() + r
        k_0 = p[0] * state[2] * np.cos(state[0])
        k_1 = p[3] * state[2] * np.cos(state[0])
        k_2 = p[2] * np.sin(state[0])
        return np.array([k_0, k_1, k_2]) / s

    def update_state(self, measurement):
        state = self.x
        y = measurement - state[2] * np.sin(state[0])
        self.x = state + y * self.gain()

    def update_p(self):
        p = self.p
        r = self.R
        a = self.alpha()
        self.p = (1 - a / (a + r)) * p

    def filter(self, measurement):
        # Prediction
        self.predict_state()
        self.predict_p()
        # Update
        self.update_state(measurement)
        self.update_p()
        # Only let the phase run between 0 and 2pi
        if self.x[0] > 2 * np.pi:
            self.x[0] = self.x[0] - 2 * np.pi
        return self.x, self.p
